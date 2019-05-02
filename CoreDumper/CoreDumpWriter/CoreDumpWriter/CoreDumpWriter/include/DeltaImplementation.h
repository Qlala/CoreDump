#pragma once
#define DELTA_THRESHOLD 0.01
#define DELTA_WINDOW 1024
void cdDelta_SetImpl(CoreDumpTop * cdtptr);

void cdDelta_CleanTop(CoreDumpTop * cdtptr);
