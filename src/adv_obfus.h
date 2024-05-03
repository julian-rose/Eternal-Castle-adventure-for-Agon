/*
 * Module: adv_obfus.h
 *
 * Purpose: Obfuscate public names when building for release
 *
 * Author: Copyright (c) julian rose, jhr
 *
 * Version:	who	when	what
 *		---	------	----------------------------------------
 *		jhr	150417	wrote initial version
 *				test data structure ideas
 */

/*
 ****************************** GLOBALS
 */
/* hide the global names from objdump etc. */

#if !defined( DEBUG )
/*!!! DON'T OBFUSCATE ANYTHING IN adv_main.c !!!*/

#define adv__welcome a1
#define adv__goodbye a2
#define adv__print_welcome a3
#define adv__print_goodbye a4
#define adv__game_over a5

#define adv__new a6
#define adv__load a7
#define adv__save a8
#define adv__read a9
#define adv__write a10
#define adv__print_error a11

#define display_initial_help a12
#define set_inbuf a13
#define adv__perform_input a14
#define adv__get_location a15

#define adv_print_buf a16
#define adv_print a17
#define adv_strlen a18
#define adv_strcpy a19
#define adv_strcmpi a20
#define adv_strstr a21
#define flush_line a22
#define adv_words a23

#define adv_world a24
#define world_and_avatar a25
#define adv_default_world a26
#define adv_default_wilbury a27
#define adv_default_nelly a28
#define adv_default_lucy a29
#define adv_default_buster a30
#define adv_default_charlie a31
#define adv_default_gary a32
#define adv_default_lofty a33
#define adv_default_otto a34
#define adv_default_harry a35
#define adv__read_file a36
#define adv__write_file a37

#define inputs a38
#define empty_object a39
#define inbuf a40
#define verb a41
#define adverb a42
#define noun a43
#define nadj a44
#define cavatar a45
#define containers a46
#define get_words a47
#define strip_word a48
#define get_verb_and_noun a49
#define get_verb_and_string a50
#define get_vowel_of_object a51

#define find_code a52
#define find_action a53
#define adv__encode_name a54

#define find_space_in_cell a55
#define find_object_in_cell a56
#define find_object_in_world a57
#define find_object_with_avatar a58
#define find_noun_as_cell_in_current a59
#define find_noun_as_cell_in_world a60
#define find_noun_in_cell a61
#define find_noun_as_object_in_current a62
#define find_noun_as_object_in_world a63
#define find_noun_as_object_with_avatars a64
#define find_noun_as_avatar a65
#define find_noun_as_avatar_rtol a66
#define find_noun_as_builtin a67
#define test_and_set_verb a68

#define find_selected a69
#define test_inventory a70
#define test_inventory_selected a71
#define set_inventory_selected a72
#define test_inventory_worn a73
#define test_inventory_free_space a74
#define give_object a75
#define take_object a76
#define test_cell_selected a77
#define set_cell_selected a78
#define container_test a79

#define make_fix a80
#define break_fix a81
#define test_fix a82

#define adv__print_status a83
#define adv__look_around a84
#define do_periodic_checks a85
#define do_avatar_mobile_checks a86

#define check_avatars a87
#define swap_avatar a88
#define distance a89
#define print_inventory a90
#define print_wearing a91
#define print_object_state a92
#define print_object_desc a93
#define print_adjective a94
#define print_cell_desc a95
#define print_cell_objects a96
#define print_cell_avatars a97

#define get_lightbeam_state a98
#define get_lightbeam_colour a99
#define lightbeam_check a100
#define print_lightbeam_state a101
#define lightbeam_event a102
#define lightbeam_test a103
#define energy_test a104
#define mirror_test a105
#define sun_test a106
#define magnotron_test a107
#define print_meter_state a108
#define get_rotary_switch_state a109
#define print_rotary_switch_state a110
#define set_rotary_switch a111
#define set_slider_switch a112
#define set_element_state a113
#define print_element_state a114

#define make_stuff a115
#define make_board a116
#define get_count_of_boards_in_world a117

#define a_ask a118
#define a_break a119
#define a_builtin a120
#define a_close a121
#define a_drink a122
#define a_eat a123
#define a_exchange a124
#define a_find a125
#define a_fill a126
#define a_fix a127
#define a_follow a128
#define a_give a129
#define a_go a130
#define a_hide a131
#define a_inventory a132
#define a_listen a133
#define a_lock a134
#define a_look a135
#define a_make a136
#define a_move a137
#define a_navigation a138
#define a_open a139
#define a_play a140
#define a_press a141
#define a_push a142
#define a_read a143
#define a_say a144
#define a_set a145
#define a_show a146
#define a_sit a147
#define a_smell a148
#define a_stand a149
#define a_swim a150
#define a_take a151
#define a_touch a152
#define a_turn a153
#define a_unlock a154
#define a_use a155
#define a_wear a156
#define a_write a157

#define actions a158
#define verbs a159
#define names a160
#define discards a161
#define state_descriptors a162

#define increment_inputs a163

#define adv_avatar a164
#define fullGameEnabled a165
#define fullGameLimitHit a166
#define adv__get_input a167
#define prev_lightbeam a168
#define pbuf a169
#define pbuf_idx a170
#define world_file a171
#define adv__print a172
#define throw a173
#define on_exit a174
#define outbuf a175
#define screen_width a176

#define adv__test_input a177
#define adv__test_input_verb a178
#define adv__test_input_adverb a179
#define adv__test_input_noun a180


#endif /* DEBUG */

