BOOL EXPENTRY InitDLL2 (HWND hwnd);
BOOL EXPENTRY InitDLL (HAB hab);
BOOL EXPENTRY StartInputHook (void);
BOOL EXPENTRY StartSendHook (void);
BOOL EXPENTRY StopHooks (void);
BOOL EXPENTRY InputProc (HAB hab, PQMSG pqMsg, ULONG fs);
VOID EXPENTRY SendProc (HAB hab,PSMHSTRUCT psmh,BOOL fInterTask);
BOOL PostMsg (HWND h, ULONG msg, MPARAM mp1, MPARAM mp2);
