
void start_skipping_cutscene ();
void check_skip_cutscene_keypress (int kgn);
void initialize_skippable_cutscene();
void stop_fast_forwarding();
void SkipUntilCharacterStops(int cc);
void EndSkippingUntilCharStops();
// skipwith decides how it can be skipped:
// 1 = ESC only
// 2 = any key
// 3 = mouse button
// 4 = mouse button or any key
// 5 = right click or ESC only
void StartCutscene (int skipwith);
int EndCutscene ();
int Game_GetSkippingCutscene();
int Game_GetInSkippableCutscene();

