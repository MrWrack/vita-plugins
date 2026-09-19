#include "ui_font.h"
vita2d_pgf *ui_font_load(void){ return vita2d_load_default_pgf(); }
void ui_font_free(vita2d_pgf *font){ if(font) vita2d_free_pgf(font); }
