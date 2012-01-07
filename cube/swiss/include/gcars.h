#ifndef GCARS_H_
#define GCARS_H_

// GCARS-CS memory map 
#define GCARS_MEMORY_START ((void*)                   0x817F6800)
#define ENTRYPOINT         ((void (*)())              0x817F6800)
#define GCARS_WRITER       ((void (*)(u32*))          0x817F6804)
#define GCARS_READER       ((void (*)(u32*))          0x817F6808)
#define GCARS_PAUSE        (*(volatile unsigned int*) 0x817FDFF4)
#define GCARS_ENABLE_CS    (*(volatile unsigned int*) 0x817FDFF8)
#define GCARS_CONDITIONAL  (*(volatile unsigned int*) 0x817FDFFC)
#define GCARS_CODELIST     ((unsigned int*)           0x817FE000)
#define MEMORY_TOP         ((void*)                   0x817FE800)

#endif
