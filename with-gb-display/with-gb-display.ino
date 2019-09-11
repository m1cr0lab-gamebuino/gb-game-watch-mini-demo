// -------------------------------------------------------------------------
// Game & Watch Mini Demo © 2019 Steph (https://gamebuino.com/@steph)
// -------------------------------------------------------------------------
// This version is based on the standard use of `gb.display` with 160x128
// indexed color display mode.
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
#include "palette.h"
#include "background.h"
#include "spritesheet.h"

// -------------------------------------------------------------------------
// Global constants
// -------------------------------------------------------------------------

const uint8_t  SCREEN_WIDTH  = 160;
const uint8_t  SCREEN_HEIGHT = 128; 

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
// Initialization of the META
// -------------------------------------------------------------------------

void setup() {
    // initializes the META
    gb.begin();
    // initializes the color palette of assets
    gb.display.setPalette(PALETTE);
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
    drawBackground();
    drawSprite(mario);
    drawSprite(monkey[player.sprite_index]);
}

void drawBackground() {
    Image colormap = Image(BACKGROUND);
    gb.display.drawImage(0, 0, colormap);
}

void drawSprite(Sprite sprite) {
    Image colormap = Image(SPRITESHEET);
    gb.display.drawImage(sprite.x, sprite.y, colormap, sprite.x, sprite.y, sprite.w, sprite.h);
}