
// BannerData Array
typedef u16 BannerData[8][24][4][4];

// RGB Structure - B must be first for correct BMP output
typedef struct {
	u8 B;
	u8 G;
	u8 R;
} RGBPoint;

// RGB With alpha bool
typedef struct {
	RGBPoint RGB;
	bool Alpha;
} RGBAPoint;

// YCbYCrPoint structure
typedef struct {
	u8 Y1;
	u8 Cb;
	u8 Y2;
	u8 Cr;
} YCbYCrPoint;

// Pixels in image 96 cols * 32 lines
#define BannerPixels 3072

// Shared function pre declaration
void ConvertBanner(BannerData RGB5A1Array, RGBPoint BgColor,int x, int y, int zoom);
void showBanner(int x, int y, int zoom);
void drawBitmap(const unsigned long* image, int scrX, int scrY, int w, int h);

