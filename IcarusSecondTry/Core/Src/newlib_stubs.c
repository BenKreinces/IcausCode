/*
 * Minimal newlib initialization hooks for the bare-metal startup path.
 *
 * The STM32 startup assembly owns reset and calls main() directly, so no C
 * runtime crt0 object supplies these hooks.  They are intentionally empty:
 * constructors, when present, are not used by this C-only firmware.
 */
void _init(void)
{
}

void _fini(void)
{
}
