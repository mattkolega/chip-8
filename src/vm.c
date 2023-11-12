#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"

void initContext(Chip8Context **chip8Context, char* romFilename) {
    *chip8Context = calloc(1, sizeof(Chip8Context));
    loadRom(romFilename, (*chip8Context)->memory);
    (*chip8Context)->pc = 512;
}

void loadRom(char *filename, uint8_t *memory) {
    FILE *fptr;

    fptr = fopen(filename, "rb");

    if (fptr == NULL) {
        printf("Error opening Chip-8 rom.");
        exit(EXIT_FAILURE);
    }

    fseek(fptr, 0l, SEEK_END);
    long fileSize = ftell(fptr);
    rewind(fptr);

    unsigned char buffer[fileSize];

    fread(buffer, sizeof(char), fileSize, fptr);

    size_t memoryIndex = 512;

    for (size_t i = 0; i < fileSize; i++) {
        memory[memoryIndex] = buffer[i];
        memoryIndex++;
    }
}

void fetchExecuteCycle(Chip8Context **chip8Context) {
    // Fetch
    uint8_t nibbleOne = (*chip8Context)->memory[(*chip8Context)->pc];
    uint8_t nibbleTwo = (*chip8Context)->memory[(*chip8Context)->pc + 1];
    uint16_t instruction = nibbleOne << 8 | nibbleTwo;
    (*chip8Context)->pc += 2;

    // Decode and Execute
    switch ((instruction & 0xF000) >> 12) {  // Check first nibble of instruction
        case 0x0:
            switch (instruction & 0x00FF) {  // Check second half of instruction
                case 0xE0:
                    Op_00E0((*chip8Context)->display);
                    break;
                case 0xEE:
                    Op_00EE(&(*chip8Context)->pc, (*chip8Context)->stack, &(*chip8Context)->sp);
                    break;
            }
            break;
        case 0x1:
            Op_1NNN(&(*chip8Context)->pc, (instruction & 0x0FFF));
            break;
        case 0x2:
            Op_2NNN(&(*chip8Context)->pc, (*chip8Context)->stack, &(*chip8Context)->sp, (instruction & 0x0FFF));
            break;
        case 0x3:
            Op_3XNN(&(*chip8Context)->pc, (*chip8Context)->v, ((instruction & 0x0F00) >> 8), (instruction & 0x00FF));
            break;
        case 0x4:
            Op_4XNN(&(*chip8Context)->pc, (*chip8Context)->v, ((instruction & 0x0F00) >> 8), (instruction & 0x00FF));
            break;
        case 0x5:
            Op_5XY0(&(*chip8Context)->pc, (*chip8Context)->v, instruction);
            break;
        case 0x6:
            Op_6XNN((*chip8Context)->v, ((instruction & 0x0F00) >> 8), (instruction & 0x00FF));
            break;
        case 0x7:
            Op_7XNN((*chip8Context)->v, ((instruction & 0x0F00) >> 8), (instruction & 0x00FF));
            break;
        case 0x8:
            switch (instruction & 0x000F) {  // Check last nibble of instruction
                case 0x0:
                    Op_8XY0((*chip8Context)->v, (instruction & 0x0F00) >> 8, (instruction & 0x00F0) >> 4);
                    break;
                case 0x1:
                    Op_8XY1((*chip8Context)->v, (instruction & 0x0F00) >> 8, (instruction & 0x00F0) >> 4);
                    break;
                case 0x2:
                    Op_8XY2((*chip8Context)->v, (instruction & 0x0F00) >> 8, (instruction & 0x00F0) >> 4);
                    break;
                case 0x3:
                    Op_8XY3((*chip8Context)->v, (instruction & 0x0F00) >> 8, (instruction & 0x00F0) >> 4);
                    break;
                case 0x4:
                    Op_8XY4((*chip8Context)->v, (instruction & 0x0F00) >> 8, (instruction & 0x00F0) >> 4);
                    break;
                case 0x5:
                    Op_8XY5((*chip8Context)->v, (instruction & 0x0F00) >> 8, (instruction & 0x00F0) >> 4);
                    break;
                case 0x6:
                    Op_8XY6((*chip8Context)->v, (instruction & 0x0F00) >> 8);
                    break;
                case 0x7:
                    Op_8XY7((*chip8Context)->v, (instruction & 0x0F00) >> 8, (instruction & 0x00F0) >> 4);
                    break;
                case 0xE:
                    Op_8XYE((*chip8Context)->v, (instruction & 0x0F00) >> 8);
                    break;
            }
            break;
        case 0x9:
            Op_9XY0(&(*chip8Context)->pc, (*chip8Context)->v, instruction);
            break;
        case 0xA:
            Op_ANNN(&(*chip8Context)->index, (instruction & 0x0FFF));
            break;
        case 0xB:
            Op_BNNN(&(*chip8Context)->pc, (*chip8Context)->v, (instruction & 0x0FFF));
            break;
        case 0xC:
            Op_CXNN((*chip8Context)->v, ((instruction & 0x0F00) >> 8), (instruction & 0x00FF));
            break;
        case 0xD:
            Op_DXYN(chip8Context, instruction);
            break;
        case 0xF:
            switch ((instruction & 0x00F0) >> 4) {
                case 0x0:
                    switch (instruction & 0x000F) {
                        case 0x7:
                            break;
                        case 0xA:
                            break;
                    }
                    break;
                case 0x1:
                    switch (instruction & 0x000F) {
                        case 0x5:
                            break;
                        case 0x8:
                            break;
                        case 0xE:
                            Op_FX1E((*chip8Context)->v, &(*chip8Context)->index, (instruction & 0x0F00) >> 8);
                            break;
                    }
                    break;
                case 0x2:
                    break;
                case 0x3:
                    Op_FX33(chip8Context, (instruction & 0x0F00) >> 8);
                    break;
                case 0x5:
                    Op_FX55(chip8Context, (instruction & 0x0F00) >> 8);
                    break;
                case 0x6:
                    Op_FX65(chip8Context, (instruction & 0x0F00) >> 8);
                    break;
            }
            break;
    }
}

void Op_00E0(bool display[][64]) {
    memset(display, 0, (32 * 64) * sizeof(bool));
}

void Op_00EE(uint16_t *pc, uint16_t *stack, uint8_t *sp) {
    (*sp)--;
    *pc = stack[*sp];
}

void Op_1NNN(uint16_t *pc, uint16_t address) {
    *pc = address;
}

void Op_2NNN(uint16_t *pc, uint16_t *stack, uint8_t *sp, uint16_t address) {
    stack[*sp] = *pc;
    (*sp)++;
    *pc = address;
}

void Op_3XNN(uint16_t *pc, uint8_t *v, uint8_t registerIndex, uint8_t value) {
    if (v[registerIndex] == value)
        *pc += 2;
}

void Op_4XNN(uint16_t *pc, uint8_t *v, uint8_t registerIndex, uint8_t value) {
    if (v[registerIndex] != value)
        *pc += 2;
}

void Op_5XY0(uint16_t *pc, uint8_t *v, uint16_t instruction) {
    int xIndex = (instruction & 0x0F00) >> 8;
    int yIndex = (instruction & 0x00F0) >> 4;

    if (v[xIndex] == v[yIndex])
        *pc += 2;
}

void Op_6XNN(uint8_t *v, uint8_t registerIndex, uint8_t value) {
    v[registerIndex] = value;
}

void Op_7XNN(uint8_t *v, uint8_t registerIndex, uint8_t value) {
    v[registerIndex] += value;
}

void Op_8XY0(uint8_t *v, int xRegisterIndex, int yRegisterIndex) {
    v[xRegisterIndex] = v[yRegisterIndex];
}

void Op_8XY1(uint8_t *v, int xRegisterIndex, int yRegisterIndex) {
    v[xRegisterIndex] |= v[yRegisterIndex];
}

void Op_8XY2(uint8_t *v, int xRegisterIndex, int yRegisterIndex) {
    v[xRegisterIndex] &= v[yRegisterIndex];
}

void Op_8XY3(uint8_t *v, int xRegisterIndex, int yRegisterIndex) {
    v[xRegisterIndex] ^= v[yRegisterIndex];
}

void Op_8XY4(uint8_t *v, int xRegisterIndex, int yRegisterIndex) {
    uint8_t sum = v[xRegisterIndex] + v[yRegisterIndex];
    v[xRegisterIndex] = sum;
    if (sum > 255)  // Test for overflow
        v[0xF] = 1;
    else
        v[0xF] = 0;
}

void Op_8XY5(uint8_t *v, int xRegisterIndex, int yRegisterIndex) {
    v[xRegisterIndex] -= v[yRegisterIndex];
    if (v[xRegisterIndex] > v[yRegisterIndex])
        v[0xF] = 1;
    else
        v[0xF] = 0;
}

void Op_8XY6(uint8_t *v, int xRegisterIndex) {
    v[0xF] = (v[xRegisterIndex] & 0b00000001);  // Set VF to value of bit to be shifted
    v[xRegisterIndex] >>= 1;
}

void Op_8XY7(uint8_t *v, int xRegisterIndex, int yRegisterIndex) {
    v[xRegisterIndex] = v[yRegisterIndex] - v[xRegisterIndex];
    if (v[yRegisterIndex] > v[xRegisterIndex])
        v[0xF] = 1;
    else
        v[0xF] = 0;
}

void Op_8XYE(uint8_t *v, int xRegisterIndex) {
    v[0xF] = (v[xRegisterIndex] & 0b10000000) >> 7;  // Set VF to value of bit to be shifted
    v[xRegisterIndex] <<= 1;
}

void Op_9XY0(uint16_t *pc, uint8_t *v, uint16_t instruction) {
    int xIndex = (instruction & 0x0F00) >> 8;
    int yIndex = (instruction & 0x00F0) >> 4;

    if (v[xIndex] != v[yIndex])
        *pc += 2;
}

void Op_ANNN(uint16_t *index, uint16_t address) {
    *index = address;
}

void Op_BNNN(uint16_t *pc, uint8_t *v, uint16_t address) {
    *pc = address + v[0];
}

void Op_CXNN(uint8_t *v, uint8_t registerIndex, uint8_t value) {
    int randNum = rand();
    int result = randNum & value;  // binary AND randNum with NN value

    v[registerIndex] = result;
}

void Op_DXYN(Chip8Context **chip8Context, uint16_t instruction) {
    int xIndex = (instruction & 0x0F00) >> 8;
    int yIndex = (instruction & 0x00F0) >> 4;

    int xCoord = (*chip8Context)->v[xIndex] % 64;  // Make starting xCoord wrap around
    int yCoord = (*chip8Context)->v[yIndex] % 32;  // Make starting yCoord wrap around

    int spriteRows = instruction & 0x000F;  // Get number of rows to draw for sprite

    (*chip8Context)->v[0xF] = 0;  // Set VF register to 0

    for (int n = 0; n < spriteRows; n++) {
        int currentYCoord = yCoord + n;  // Increment yCoord for each row
        if (currentYCoord > 31) break;
        uint8_t spriteByte = (*chip8Context)->memory[(*chip8Context)->index + n];
        uint8_t bitmask = 0b10000000;
        uint8_t bitshiftAmount = 7;

        for (int i = 0; i < 8; i++) {
            int currentXCoord = xCoord + i;  // Increment xCoord for each column
            if (currentXCoord > 63) break;
            uint8_t spriteBit = (spriteByte & bitmask) >> bitshiftAmount;
            bitmask = bitmask >> 1;
            bitshiftAmount -= 1;

            if (spriteBit ^ (*chip8Context)->display[currentYCoord][currentXCoord]) {
                (*chip8Context)->display[currentYCoord][currentXCoord] = true;
            } else {
                (*chip8Context)->display[currentYCoord][currentXCoord] = false;
                (*chip8Context)->v[0xF] = 1;  // Set VF register to 1
            }
        }
    }
}

void Op_FX1E(uint8_t *v, uint16_t *index, uint8_t registerIndex) {
    *index += v[registerIndex];
}

void Op_FX33(Chip8Context **chip8Context, uint8_t registerIndex) {
    uint8_t number = (*chip8Context)->v[registerIndex];

    uint8_t numberDigits[3];
    numberDigits[0] = (number / 100) % 10;
    numberDigits[1] = (number / 10) % 10;
    numberDigits[2] = number % 10;

    for (int i = 0; i < 3; i++) {
        (*chip8Context)->memory[(*chip8Context)->index + i] = numberDigits[i];
    }
}

void Op_FX55(Chip8Context **chip8Context, uint8_t registerIndex) {
    for (int i = 0; i <= registerIndex; i++) {
        (*chip8Context)->memory[(*chip8Context)->index + i] = (*chip8Context)->v[i];
    }
    (*chip8Context)->index += registerIndex + 1;
}

void Op_FX65(Chip8Context **chip8Context, uint8_t registerIndex) {
    for (int i = 0; i <= registerIndex; i++) {
        (*chip8Context)->v[i] = (*chip8Context)->memory[(*chip8Context)->index + i];
    }
    (*chip8Context)->index += registerIndex + 1;
}