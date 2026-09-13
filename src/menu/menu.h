#pragma once

// Encoder-driven menu. initMenu() draws the initial screen; menuUpdate() should
// be called frequently from the main loop to read the encoder and redraw.
void initMenu();
void menuUpdate();
