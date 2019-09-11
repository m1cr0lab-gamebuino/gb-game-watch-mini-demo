# Game & Watch Mini Demo for the Gamebuino META

This code is a mini-project to consider as a starting point for the implementation of a High Resolution game for the Gamebuino META, inspired by Nintendo's Game & Watch console series.

It responds to [a request from Jicehel](https://community.gamebuino.com/t/demande-de-squelette/943) made on the official Gamebuino forum. I wrote it in haste to help [Jicehel](https://gamebuino.com/@jicehel) realize the project of his dreams...

If time allows, and assuming that a strong demand is expressed by the community, I can consider writing a complete tutorial on the subject...

## Implementation

Two versions are proposed here to illustrate two different ways to perform the graphic rendering calculation with the Gamebuino META:

- The first version is based on the standard use of `gb.display` with `160x128` indexed color display mode.
- The second version does not rely on the use of `gb.display`, but rather uses the low-level `gb.tft` API to fully exploit the RGB565 color space (although it is not necessary here). The graphic rendering is cleverly calculated on two partial framebuffers,
which are alternately transmitted to the DMA controller, which acts as
an intermediary with the display device.

## Acknowledgements

- Many thanks to [Andy O'Neill](https://gamebuino.com/@aoneill) for the magical routines related to the DMA controller (very useful to tackling `160x128` High Definition on the Gamebuino META).

- Assets come from http://www.mariouniverse.com/.  
  Thanks to « Lotos » for sharing.

## Demo

![game demo](https://community.gamebuino.com/uploads/default/original/1X/83a8947efd9a6c40411def15ed381b50bd1f9747.gif)