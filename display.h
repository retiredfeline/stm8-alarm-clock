enum display_mode { D_NONE = 0, D_HOURS = 0x1, D_MINS = 0x2, };

extern uint8_t display_mode;

extern void display_init(void);
extern void display_flip_b5(void);
extern void display_set_ptr(enum display_mode);
extern void display_update(void);
extern void display_update_dot(void);
extern void display_set_visibility(uint8_t);
extern void display_next_digit(void);
extern uint8_t display_digit_number(void);
