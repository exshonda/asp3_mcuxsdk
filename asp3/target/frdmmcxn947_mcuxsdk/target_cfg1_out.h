/*
 *		cfg1_out.cをリンクするために必要なスタブの定義
 */

void Reset_Handler(void) {}

int main() { return 0; }

/*
 *  チップ依存のスタブの定義
 */
#include <chip_cfg1_out.h>
