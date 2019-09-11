// -------------------------------------------------------------------------
// Game & Watch Mini Demo © 2019 Steph (https://gamebuino.com/@steph)
// -------------------------------------------------------------------------
// This version does not rely on the use of `gb.display`, but rather uses
// the low-level `gb.tft` API to fully exploit the RGB565 color space
// (although it is not necessary here).
// 
// The graphic rendering is cleverly calculated on two partial framebuffers,
// which are alternately transmitted to the DMA controller, which acts as
// an intermediary with the display device.
// 
// Many thanks to Andy (https://gamebuino.com/@aoneill) for the magical
// routines related to the DMA controller (very useful to tackling 160x128
// High Definition on the META).
// -------------------------------------------------------------------------
// Assets come from http://www.mariouniverse.com/
// Thanks to « Lotos » for sharing.
// -------------------------------------------------------------------------
// This code is a starting point for the implementation of games inspired by
// Nintendo's Game & Watch console series, in High Definition on the META.
// I wrote it in haste to help Jicehel realize the project of his dreams...
// => To watch for on https://gamebuino.com/@jicehel :)
// -------------------------------------------------------------------------

// includes the official library
#include <Gamebuino-Meta.h>

// includes assets
#include "background.h"
#include "spritesheet.h"

// -------------------------------------------------------------------------
// Global constants
// -------------------------------------------------------------------------

const uint8_t  SCREEN_WIDTH  = 160;
const uint8_t  SCREEN_HEIGHT = 128; 
const uint8_t  SLICE_HEIGHT  = 8;
const uint16_t TRANS_COLOR   = 0xfbd6;

// -------------------------------------------------------------------------
// Sprites
// -------------------------------------------------------------------------
// The surface of the spritesheet is deliberately not optimized here to
// facilitate positioning calculations at the time of rendering.
// -------------------------------------------------------------------------

// each sprite is characterized by its dimensions and coordinates
// on the spritesheet
struct Sprite {
    uint8_t w, h;
    uint8_t x, y;
};

// a static sprite
Sprite mario = { 15, 23, 3, 34 };

// the different postures and positions of the player's avatar
Sprite monkey[] = {
    { 19, 20,  19, 100 },  //  0
    { 22, 20,  41, 100 },  //  1
    { 20, 20,  64, 100 },  //  2
    { 21, 20,  89, 100 },  //  3
    { 21, 20, 110, 100 },  //  4
    { 21, 20, 131, 100 },  //  5
    { 16, 18,  21,  81 },  //  6
    { 22, 19,  41,  81 },  //  7
    { 15, 18,  66,  81 },  //  8
    { 15, 17,  90,  81 },  //  9
    { 21, 18, 108,  81 },  // 10
    { 20, 20, 130,  79 },  // 11
    { 17, 20,  68,  52 },  // 12
    { 19, 19,  90,  52 },  // 13
    { 20, 20, 109,  51 },  // 14
    { 19, 19, 129,  54 },  // 15
    { 15, 18, 112,  33 }   // 16
};

// -------------------------------------------------------------------------
// Behavioral transition tables of the player's avatar
// -------------------------------------------------------------------------
// The recorded values correspond to the indexes of the postures of the
// `monkey' sprite (and -1 indicates an impossible transition).
// -------------------------------------------------------------------------

int8_t to_left[] = {
//   0   1   2   3   4   5   //  |  from 
    -1,  0,  1,  2,  3,  4,  //  ▼  to

//   6   7   8   9  10  11
    -1, -1, -1,  8, -1, -1,
    
//  12  13  14  15
    -1, 12, 13, 14,

//  16
    -1
};

int8_t to_right[] = {
//   0   1   2   3   4   5
     1,  2,  3,  4,  5, -1,

//   6   7   8   9  10  11
    -1, -1,  9, -1, -1, -1,
    
//  12  13  14  15
    13, 14, 15, -1,

//  16
    -1
};

int8_t climbup[] = {
//   0   1   2   3   4   5
    -1, -1, -1, -1, -1, -1,

//   6   7   8   9  10  11
    -1, -1, -1, -1, -1, 15,
    
//  12  13  14  15
    -1, -1, -1, -1,

//  16
    -1
};

int8_t jump[] = {
//   0   1   2   3   4   5
     6,  7,  8,  9, 10, 11,

//   6   7   8   9  10  11
    -1, -1, -1, -1, -1, -1,
    
//  12  13  14  15
    -1, -1, 16, -1,

//  16
    -1
};

int8_t comedown[] = {
//   0   1   2   3   4   5
    -1, -1, -1, -1, -1, -1,

//   6   7   8   9  10  11
    -1, -1, -1, -1, -1, -1,
    
//  12  13  14  15
    -1, -1, -1, 11,

//  16
    -1
};

int8_t fall[] = {
//   0   1   2   3   4   5
    -1, -1, -1, -1, -1, -1,

//   6   7   8   9  10  11
     0,  1,  2,  3,  4,  5,
    
//  12  13  14  15
    -1, -1, -1, -1,

//  16
    14
};

// -------------------------------------------------------------------------
// Player definition
// -------------------------------------------------------------------------

struct Player {
    uint8_t  sprite_index;
    bool     jumping;
    uint32_t jump_timer;
};

Player player = {
    0,      // sprite_index
    false,  // jumping
    0       // jump_timer
};

// -------------------------------------------------------------------------
// Initialization related to the DMA controller
// -------------------------------------------------------------------------

namespace Gamebuino_Meta {
    #define DMA_DESC_COUNT 3
    extern volatile uint32_t dma_desc_free_count;
    static inline void wait_for_transfers_done(void) {
        while (dma_desc_free_count < DMA_DESC_COUNT);
    }
    static SPISettings tftSPISettings = SPISettings(24000000, MSBFIRST, SPI_MODE0);
};

// rendering buffers
uint16_t buffer1[SCREEN_WIDTH * SLICE_HEIGHT];
uint16_t buffer2[SCREEN_WIDTH * SLICE_HEIGHT];

// flag for an ongoing data transfer
bool drawPending = false;

// -------------------------------------------------------------------------
// Initialization of the META
// -------------------------------------------------------------------------

void setup() {
    // initializes the META
    gb.begin();
    // default screen buffer won't be used
    // so it must be set to 0x0 pixels
    gb.display.init(0, 0, ColorMode::rgb565);
}

// -------------------------------------------------------------------------
// Main control loop
// -------------------------------------------------------------------------

void loop() {
    gb.waitForUpdate();
    // a classic sequence :)
    getUserInput();
    update();
    draw();
}

// -------------------------------------------------------------------------
// User interaction
// -------------------------------------------------------------------------

void getUserInput() {
    if      (gb.buttons.pressed(BUTTON_LEFT))  movePlayer(to_left);
    else if (gb.buttons.pressed(BUTTON_RIGHT)) movePlayer(to_right);
    else if (gb.buttons.pressed(BUTTON_UP))    movePlayer(climbup);
    else if (gb.buttons.pressed(BUTTON_DOWN))  movePlayer(comedown);

    else if (gb.buttons.pressed(BUTTON_A)) {
        if (player.sprite_index == 1 || player.sprite_index == 4) {
            player.jumping = true;
            player.jump_timer = gb.frameCount;
        }
        movePlayer(jump);
    }

    else if (gb.buttons.pressed(BUTTON_B) && !player.jumping) {
        movePlayer(fall);
    }
}

// -------------------------------------------------------------------------
// Handling of the player's avatar movements
// -------------------------------------------------------------------------

void movePlayer(int8_t* direction) {
    int8_t next = direction[player.sprite_index];
    if (next != -1) player.sprite_index = next;
}

// -------------------------------------------------------------------------
// Game logic
// -------------------------------------------------------------------------

void update() {
    // here we simply handle the jump of the player's avatar
    // when he hasn't clung to a vine
    if (player.jumping && gb.frameCount - player.jump_timer > 12) {
        player.jumping = false;
        movePlayer(fall);
    }
}

// -------------------------------------------------------------------------
// Graphic rendering
// -------------------------------------------------------------------------

void draw() {
    // the number of horizontal slices to be cut is calculated
    uint8_t slices = SCREEN_HEIGHT / SLICE_HEIGHT;
    // declares a pointer that will alternate between the two memory buffers
    uint16_t* buffer;
    // declares the top border of current slice
    uint8_t sliceY;
    // then we go through each slice one by one
    for (uint8_t sliceIndex = 0; sliceIndex < slices; sliceIndex++) {
        // buffers are switched according to the parity of sliceIndex
        buffer = sliceIndex % 2 == 0 ? buffer1 : buffer2;
        // the top border of the current slice is calculated
        sliceY = sliceIndex * SLICE_HEIGHT;

        // starts by drawing the background
        memcpy(buffer, BACKGROUND + sliceY * SCREEN_WIDTH, 2 * SCREEN_WIDTH * SLICE_HEIGHT);

        // then draws Mario (a static sprite)
        drawSprite(mario, sliceY, buffer);

        // and finally draws the player's avatar
        drawSprite(monkey[player.sprite_index], sliceY, buffer);

        // then we make sure that the sending of the previous buffer
        // to the DMA controller has taken place
        if (sliceIndex != 0) waitForPreviousDraw();
        // after which we can then send the current buffer
        customDrawBuffer(0, sliceY, buffer, SCREEN_WIDTH, SLICE_HEIGHT);
    }

    // always wait until the DMA transfer is completed
    // for the last slice before entering the next cycle
    waitForPreviousDraw();
}

void drawSprite(Sprite sprite, uint8_t sliceY, uint16_t* buffer) {
    // we check first of all that the intersection between
    // the sprite and the current slice is not empty
    if (sliceY < sprite.y + sprite.h && sprite.y < sliceY + SLICE_HEIGHT) {
        // determines the boundaries of the sprite surface within the current slice
        uint8_t  xmin = sprite.x;
        uint8_t  xmax = sprite.x + sprite.w - 1;
        uint8_t  ymin = sprite.y < sliceY ? sliceY : sprite.y;
        uint8_t  ymax = sprite.y + sprite.h >= sliceY + SLICE_HEIGHT ? sliceY + SLICE_HEIGHT - 1 : sprite.y + sprite.h - 1;

        uint8_t  px, py;
        uint16_t color;
        // goes through the sprite pixels to be drawn
        for (py = ymin; py <= ymax; py++) {
            for (px = xmin; px <= xmax; px++) {
                // picks the pixel color from the spritesheet
                color = SPRITESHEET[px + py * SCREEN_WIDTH];
                // and if it is different from the transparency color
                if (color != TRANS_COLOR) {
                    // copies the color code into the rendering buffer
                    buffer[px + (py - sliceY) * SCREEN_WIDTH] = color;
                }
            }
        }
    }
}

// -------------------------------------------------------------------------
// Memory transfer to DMA controller
// -------------------------------------------------------------------------

// initiates memory forwarding to the DMA controller....
void customDrawBuffer(uint8_t x, uint8_t y, uint16_t* buffer, uint8_t w, uint8_t h) {
    drawPending = true;
    gb.tft.setAddrWindow(x, y, x + w - 1, y + h - 1);
    SPI.beginTransaction(Gamebuino_Meta::tftSPISettings);
    gb.tft.dataMode();
    gb.tft.sendBuffer(buffer, w*h);
}

// waits for the memory transfer to be completed
// and close the transaction with the DMA controller
void waitForPreviousDraw() {
    if (drawPending) {
        Gamebuino_Meta::wait_for_transfers_done();
        gb.tft.idleMode();
        SPI.endTransaction();
        drawPending = false;
    }
}