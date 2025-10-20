//---
//	render-cg - Internal definitions for the display module on fxcg50
//---

#ifndef RENDER_CG
#define RENDER_CG

/* dvram_switch() - triple buffering switch
   Alternates VRAMs after a display update started. */
void dvram_switch(void);

#endif /* RENDER_CG */
