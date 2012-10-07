//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

//extern "C" int Init_2xSaI(int d);
//extern "C" void Super2xSaI(BITMAP * src, BITMAP * dest, int s_x, int s_y, int d_x, int d_y, int w, int h);

/* 
 *** UNFORTUNATELY THE 2xSAI SCALER IS GPL AND SO WE CANNOT
 *** USE IT IN RELEASE CODE

struct Super2xSAIGFXFilter : public ScalingGFXFilter {
private:
  BITMAP *realScreenBuffer;

public:

  Super2xSAIGFXFilter(bool justCheckingForSetup) : ScalingGFXFilter(2, justCheckingForSetup) { }

  virtual const char* Initialize(int width, int height, int colDepth) {
    if (colDepth == 8)
      return "8-bit colour not supported";

    return ScalingGFXFilter::Initialize(width, height, colDepth);
  }

  virtual BITMAP* ScreenInitialized(BITMAP *screen, int fakeWidth, int fakeHeight) {
    realScreen = screen;
    realScreenBuffer = create_bitmap(screen->w, screen->h);
    fakeScreen = create_bitmap_ex(bitmap_color_depth(screen), fakeWidth, fakeHeight);
    Init_2xSaI(bitmap_color_depth(screen));
    return fakeScreen;
  }

  virtual BITMAP *ShutdownAndReturnRealScreen(BITMAP *currentScreen) {
    destroy_bitmap(fakeScreen);
    destroy_bitmap(realScreenBuffer);
    return realScreen;
  }

  virtual void RenderScreen(BITMAP *toRender, int x, int y) {

    acquire_bitmap(realScreenBuffer);
    Super2xSaI(toRender, realScreenBuffer, 0, 0, 0, 0, toRender->w, toRender->h);
    release_bitmap(realScreenBuffer);

    blit(realScreenBuffer, realScreen, 0, 0, x * MULTIPLIER, y * MULTIPLIER, realScreen->w, realScreen->h);

    lastBlitFrom = toRender;
  }

  virtual const char *GetVersionBoxText() {
    return "Super2xSaI filter[";
  }

  virtual const char *GetFilterID() {
    return "Super2xSaI";
  }
};
*/
