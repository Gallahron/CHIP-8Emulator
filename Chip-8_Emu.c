#include "raylib.h"
#include "stdlib.h"
#include "math.h"
#include "stdio.h"
#include "string.h"

#define VRAM_START 0xF00
#define SCREEN_HEIGHT 32
#define SCREEN_WIDTH 64
#define FONT_START 0

struct Node {
    struct Node* next;
    struct Node* prev;
    unsigned short address;
};

struct Stack {
    struct Node* head;
    struct Node* tail;
};

typedef struct Instruction {
    unsigned short a : 4;
    unsigned short b : 4;
    unsigned short c : 4;
    unsigned short d : 4;
} Instruction;

short firstTwo(Instruction in) {
    short out = in.a << 4;
    out |= in.b;
    return out;
} 
short lastTwo(Instruction in) {
    short out = in.c << 4;
    out |= in.d;
    return out;
}
short lastThree(Instruction in) {
    short out = in.b << 8;
    out |= lastTwo(in);
    return out;
}


int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = SCREEN_WIDTH * 12;
    const int screenHeight = SCREEN_HEIGHT * 12;

    InitWindow(screenWidth, screenHeight, "CHIP-8");

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    
    unsigned char* memory = calloc(4096,1);
    unsigned char registers[16];
    short int addReg;
    char delayTimer, soundTimer;
    
    unsigned int PC = 0x200;

    float tick = 0;

    int keys[] = {KEY_ONE,KEY_TWO,KEY_THREE,KEY_FOUR,KEY_FIVE,KEY_SIX,KEY_SEVEN,KEY_EIGHT,KEY_NINE,KEY_A,KEY_B,KEY_C,KEY_D,KEY_E,KEY_F};

    char font[] = { 0xF0, 0x90, 0x90, 0x90, 0xF0,		// 0
                    0x20, 0x60, 0x20, 0x20, 0x70,		// 1
                    0xF0, 0x10, 0xF0, 0x80, 0xF0,		// 2
                    0xF0, 0x10, 0xF0, 0x10, 0xF0,		// 3
                    0x90, 0x90, 0xF0, 0x10, 0x10,		// 4
                    0xF0, 0x80, 0xF0, 0x10, 0xF0,		// 5
                    0xF0, 0x80, 0xF0, 0x90, 0xF0,		// 6
                    0xF0, 0x10, 0x20, 0x40, 0x40,		// 7
                    0xF0, 0x90, 0xF0, 0x90, 0xF0,		// 8
                    0xF0, 0x90, 0xF0, 0x10, 0xF0,		// 9
                    0xF0, 0x90, 0xF0, 0x90, 0x90,		// A
                    0xE0, 0x90, 0xE0, 0x90, 0xE0,		// B
                    0xF0, 0x80, 0x80, 0x80, 0xF0,		// C
                    0xE0, 0x90, 0x90, 0x90, 0xE0,		// D
                    0xF0, 0x80, 0xF0, 0x80, 0xF0,		// E
                    0xF0, 0x80, 0xF0, 0x80, 0x80};		// F

    for (int i = 0; i < 16 * 5; i++){
        memory[FONT_START + i] = font[i];
    }

    struct Stack* stack = malloc(sizeof(struct Stack));
    //Load ROM into memory
    {
    FILE *fp;
    char *ptr = &(memory[512]);
    fp = fopen("ROMS\\Pong [Paul Vervalin, 1990].ch8","rb");
    if (!fp) {
        printf("Error, file not opened");
        return 0;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    printf("%u",size);
    fseek(fp,0,SEEK_SET);
    fread(ptr, 1, size, fp);
    fclose(fp);
    }
    // Main game loop
    BeginDrawing();
    ClearBackground(BLACK);
    EndDrawing();

    
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        tick += GetFrameTime()*60;
        if (tick>1) {
            if (delayTimer > 0) delayTimer--;
            else delayTimer = 0;
            if (soundTimer > 0) soundTimer--;
            else soundTimer = 0;
            tick = 0;
        }
        Instruction inst;
        inst.a = memory[PC]   >> 4 & 0xf;
        inst.b = memory[PC]        & 0xf;
        inst.c = memory[PC+1] >> 4 & 0xf;
        inst.d = memory[PC+1]      & 0xf;
        unsigned short regX,regY,val,op,out;
        struct Node* new;
        //printf("PC:%x a:%x b:%x c:%x d:%x\n", PC, inst.a, inst.b, inst.c, inst.d);
        //printf("INSTRUCTION:%x%x\n", memory[PC], memory[PC+1]);
        switch (inst.a){
            case 0:
                if (lastTwo(inst) == 0xE0) {
                    BeginDrawing();
                    for (int x = 0; x < SCREEN_WIDTH; x+=8){
                        for (int y = 0; y < SCREEN_HEIGHT; y++){
                            memory[VRAM_START+y*8+x/8] = 0;
                        }
                    }
                    ClearBackground(BLACK);
                    EndDrawing();
                    //printf("CLEAR\n");
                }
                else if (lastTwo(inst) == 0xEE){
                    struct Node* stor = stack->tail;
                    PC = stor->address;
                    stack->tail = stor->prev;
                    free(stor);
                }
                break;
            case 1:
                out = lastThree(inst);
                //printf("JUMP: %x\n",out);
                PC = out-2;
                break;
            case 2:
                new = malloc(sizeof(struct Node));
                new->address = PC;
                if (!stack->head) {
                    stack->head = new;
                    stack->tail = new;
                }
                else{
                    new->prev = stack->tail;
                    stack->tail = new;
                }
                
                out = lastThree(inst);
                PC = out - 2;
                break;
            case 3:
                //printf("Comparison: reg %x == %x\n",regX, val);
                if (registers[inst.b] == lastTwo(inst)) PC+=2;
                break;
            case 4:
                //printf("Comparison: reg %x != %x\n",regX, val);
                if (registers[inst.b] != lastTwo(inst)) PC+=2;
                break;
            case 5:
                //printf("Comparison: reg %x == reg %x\n",regX, regY);
                if (registers[inst.b] == registers[inst.c]) PC+=2;
                break;
            case 6:
                //printf("SET: reg %x = %x\n",regX, val);
                registers[inst.b] = lastTwo(inst);
                break;
            case 7:
                //printf("ADD: reg %x += %x\n",regX, val);
                registers[inst.b] += lastTwo(inst);
                break;
            case 8:
                regX = inst.b;
                regY = inst.c;
                switch (inst.d) {
                    case 0:
                        registers[regX] = registers[regY];
                        break;
                    case 1:
                        registers[regX] |= registers[regY];
                        break;
                    case 2:
                        registers[regX] &= registers[regY];
                        break;
                    case 3:
                        registers[regX] ^= registers[regY];
                        break;
                    case 4:
                        out = registers[regX] + registers[regY];
                        
                        if (out < registers[regX] || out < registers[regY]) registers[0xF] = 1;
                        else registers[0xF] = 0;
                        registers[regX] = out;
                        break;
                    case 5:
                        out = registers[regX] - registers[regY];
                        
                        if (registers[regX] >= registers[regY]) registers[0xF] = 1;
                        else registers[0xF] = 0;
                        registers[regX] = out;
                        break;
                    case 6:
                        registers[0xF] = registers[regX]%2;
                        registers[regX] >>= 1;
                        break;
                    case 7:
                        out = registers[regY] - registers[regX];
                        
                        if (registers[regY] >= registers[regX]) registers[0xF] = 1;
                        else registers[0xF] = 0;
                        registers[regX] = out;
                        break;
                    case 0xE:
                        registers[0xF] = registers[regX]>>7;
                        registers[regX] <<= 1;
                        break;
                }
                break;
            case 9:
                //printf("Comparison: reg %x != reg %x\n",regX, regY);
                if (registers[inst.b] != registers[inst.c]) PC+=2;
                break;
            case 0xA:
                addReg = lastThree(inst);
                //printf("LOC: I= %x\n", out);
                break;
            case 0xB:
                PC = registers[0] + lastThree(inst);
                break;
            case 0xC:
                registers[inst.b] = rand()&lastTwo(inst);
                break;
            case 0xD:
                regX = inst.b;
                regY = inst.c;
                val = inst.d;
                out = 0;
                //printf("DRAW: (%u,%u)\n",registers[regX],registers[regY]);
                for (int y = 0; y < val; y++) {
                    
                    unsigned char a1 = memory[addReg + y]>>(registers[regX]%8);
                    unsigned char a2 = memory[addReg + y]<<((8-(registers[regX]%8)));
                    unsigned char* b1 = &memory[VRAM_START + (registers[regY]+y)*8+registers[regX]/8];
                    unsigned char* b2 = &memory[VRAM_START + (registers[regY]+y)*8+registers[regX]/8+1];
                    //memory[VRAM_START + y*8 + x/8]
                    //printf("%x %x\n",*b1,*b2);
                    //printf("%x %x %x\n",memory[addReg + y],a1,a2);
                    if (a1&*b1 > 0 || a2&*b2 > 0) out = 1;
                    *b1 ^= a1;
                    *b2 ^= a2;
                    //printf("%x %x\n",*b1,*b2);
                }
                //printf("%u\n",out);
                registers[0xF] = out;
                //memory[VRAM_START] = 0xFF;
                BeginDrawing();
                ClearBackground(BLACK);
                for (int x = 0; x < SCREEN_WIDTH; x++){
                    for (int y = 0; y < SCREEN_HEIGHT; y++) {
                        Color col;
                        if ((memory[VRAM_START + y*8 + x/8]>>(7-x%8))%2) DrawRectangle(x*12,y*12,12,12,WHITE);
                        
                    }
                }
                EndDrawing();
                break;
            case 0xE:
                printf("Checkkey %x\n", registers[inst.b]);
                if (lastTwo(inst) == 0x9E) {
                    if (IsKeyDown(keys[registers[inst.b]])){
                        PC += 2;
                    }
                }
                else if (lastTwo(inst) == 0xA1) {
                    if (!IsKeyDown(keys[registers[inst.b]])){
                        PC += 2;
                    }
                }
                break;
            case 0xF:
                regX = inst.b;
                switch (lastTwo(inst))
                {
                    case 0x07:
                        registers[regX] = delayTimer;
                        break;
                    case 0x0A:
                        //Need to implement
                        break;
                    case 0x15:
                        delayTimer = registers[regX];
                        break;
                    case 0x18:
                        soundTimer = registers[regX];
                        break;
                    case 0x1E:
                        addReg += registers[regX];
                        break;
                    case 0x29:
                        addReg = FONT_START + registers[regX]*5;
                        break;
                    case 0x33:
                        //printf("BCD: RegX = %u, ", registers[regX]);
                        out = registers[regX];
                        for (int i = 2;  i>=0; i--) {
                            memory[addReg + 2-i] = out/pow(10,i);
                            //printf("%u, ",memory[addReg + 2-i]);
                            out %= (int)pow(10,i);
                        }
                        //printf("\n");
                        break;
                    case 0x55:
                        //printf("REGDUMP: From Reg[0] to Reg[%x]\n",regX);
                        for (int i = 0; i <= regX; i++){
                            memory[addReg+i] = registers[i];
                            //printf("REGDUMP: Reg[%x] = %u\n",i, registers[i]);
                        }
                        break;
                    case 0x65:
                        //printf("REGGET: From Reg[0] to Reg[%x]\n",regX);
                        for (int i = 0; i <= regX; i++){
                            registers[i] = memory[addReg+i];
                            //printf("REGGET: Reg[%x] = %u\n",i, registers[i]);
                        }
                        break;

                }

                break;
                
        }
        PC+=2;
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
