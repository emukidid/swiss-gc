// Copyright 2017 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "DVDMath.h"

#include <math.h>

#include "common.h"

// The size of the first GC disc layer in bytes (712880 sectors, 2048 bytes per sector)
static const u32 DISC_LAYER_SIZE = 0x57058000;

// 24 mm
static const double DISC_INNER_RADIUS = 0.024;
// 38 mm
static const double DISC_OUTER_RADIUS = 0.038;
// 0.74 um
static const double DISC_TRACK_PITCH = 0.00000074;

// Approximate read speeds at the inner and outer locations of Wii and GC
// discs. These speeds are approximations of speeds measured on real Wiis.
static const double DISC_INNER_READ_SPEED = 1024 * 1024 * 2;      // bytes/s
static const double DISC_OUTER_READ_SPEED = 1024 * 1024 * 3.125;  // bytes/s

// The speed at which discs rotate. These have not been directly measured on hardware -
// rather, the read speeds above have been matched to the closest standard DVD speed
// (3x for GC and 6x for Wii) and the rotational speeds of those have been used.
static const double ROTATIONS_PER_SECOND = 28.5;

// Experimentally measured seek constants. The time to seek appears to be
// linear, but short seeks appear to be lower velocity.
static const double SHORT_SEEK_MAX_DISTANCE = 0.001;   // 1 mm
static const double SHORT_SEEK_CONSTANT = 0.035;       // seconds
static const double SHORT_SEEK_VELOCITY_INVERSE = 50;  // inverse: s/m
static const double LONG_SEEK_CONSTANT = 0.075;        // seconds
static const double LONG_SEEK_VELOCITY_INVERSE = 4.5;  // inverse: s/m

// We can approximate the relationship between a byte offset on disc and its
// radial distance from the center by using an approximation for the length of
// a rolled material, which is the area of the material divided by the pitch
// (ie: assume that you can squish and deform the area of the disc into a
// rectangle as thick as the track pitch).
//
// In practice this yields good-enough numbers as a more exact formula
// involving the integral over a polar equation (too complex to describe here)
// or the approximation of a DVD as a set of concentric circles (which is a
// better approximation, but makes futher derivations more complicated than
// they need to be).
//
// From the area approximation, we end up with this formula:
//
// L = pi*(r.outer^2-r.inner^2)/pitch
//
// Where:
//   L = the data track's physical length
//   r.{inner,outer} = the inner/outer radii (24 mm and 58 mm)
//   pitch = the track pitch (.74 um)
//
// We can then use this equation to compute the radius for a given sector in
// the disc by mapping it along the length to a linear position and inverting
// the equation and solving for r.outer (using the DVD's r.inner and pitch)
// given that linear position:
//
// r.outer = sqrt(L * pitch / pi + r.inner^2)
//
// Where:
//   L = the offset's linear position, as offset/density
//   r.outer = the radius for the offset
//   r.inner and pitch are the same as before.
//
// The data density of the disc is just the number of bytes addressable on a
// DVD, divided by the spiral length holding that data. offset/density yields
// the linear position for a given offset.
//
// When we put it all together and simplify, we can compute the radius for a
// given byte offset as a drastically simplified:
//
// r = sqrt(offset/total_bytes*(r.outer^2-r.inner^2) + r.inner^2)
double CalculatePhysicalDiscPosition(u32 offset)
{
  return sqrt((double)offset / DISC_LAYER_SIZE *
          (DISC_OUTER_RADIUS * DISC_OUTER_RADIUS - DISC_INNER_RADIUS * DISC_INNER_RADIUS) +
      DISC_INNER_RADIUS * DISC_INNER_RADIUS);
}

// Returns the time in seconds to move the read head from one offset to
// another, plus the number of ticks to read one ECC block immediately
// afterwards. Based on hardware testing, this appears to be a function of the
// linear distance between the radius of the first and second positions on the
// disc, though the head speed varies depending on the length of the seek.
double CalculateSeekTime(u32 offset_from, u32 offset_to)
{
  const double position_from = CalculatePhysicalDiscPosition(offset_from);
  const double position_to = CalculatePhysicalDiscPosition(offset_to);

  // Seek time is roughly linear based on head distance travelled
  const double distance = fabs(position_from - position_to);

  if (distance < SHORT_SEEK_MAX_DISTANCE)
    return distance * SHORT_SEEK_VELOCITY_INVERSE + SHORT_SEEK_CONSTANT;
  else
    return distance * LONG_SEEK_VELOCITY_INVERSE + LONG_SEEK_CONSTANT;
}

// Returns the time in seconds it takes for the disc to spin to the angle where the
// read head is over the given offset, starting from the given time in seconds.
double CalculateRotationalLatency(u32 offset, double time)
{
  // The data track on the disc is modelled as an Archimedean spiral.

  // We assume that the inserted disc is spinning constantly, which
  // is not always true (especially not when no disc is inserted),
  // but doesn't lead to any problems in practice. It's as if every
  // newly inserted disc is inserted at such an angle that it ends
  // up rotating identically to a disc inserted at boot.

  // To make the calculations simpler, the angles go up to 1, not tau.
  // But tau (or anything else) would also work.
  const double MAX_ANGLE = 1;

  const double target_angle =
      fmod(CalculatePhysicalDiscPosition(offset) / DISC_TRACK_PITCH * MAX_ANGLE, MAX_ANGLE);
  const double start_angle = fmod(time * ROTATIONS_PER_SECOND * MAX_ANGLE, MAX_ANGLE);

  // MAX_ANGLE is added so that fmod can't get a negative argument
  const double angle_diff = fmod(target_angle + MAX_ANGLE - start_angle, MAX_ANGLE);

  return angle_diff / MAX_ANGLE / ROTATIONS_PER_SECOND;
}

// Returns the time in seconds it takes to read an amount of data from a disc,
// ignoring factors such as seek times. This is the streaming rate of the
// drive and varies between ~3-8MiB/s for Wii discs. Note that there is technically
// a DMA delay on top of this, but we model that as part of this read time.
double CalculateRawDiscReadTime(u32 offset, u32 length)
{
  // The Wii/GC have a CAV drive and the data has a constant pit length
  // regardless of location on disc. This means we can linearly interpolate
  // speed from the inner to outer radius. This matches a hardware test.
  // We're just picking a point halfway into the read as our benchmark for
  // read speed as speeds don't change materially in this small window.
  const double physical_offset = CalculatePhysicalDiscPosition(offset + length / 2);

  const double speed =
      (physical_offset - DISC_INNER_RADIUS) / (DISC_OUTER_RADIUS - DISC_INNER_RADIUS) *
          (DISC_OUTER_READ_SPEED - DISC_INNER_READ_SPEED) +
      DISC_INNER_READ_SPEED;

  return length / speed;
}
