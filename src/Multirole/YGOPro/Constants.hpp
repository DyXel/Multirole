#ifndef YGOPRO_CONSTANTS_HPP
#define YGOPRO_CONSTANTS_HPP

#define LOCATION_DECK    0x01
#define LOCATION_HAND    0x02
#define LOCATION_MZONE   0x04
#define LOCATION_SZONE   0x08
#define LOCATION_GRAVE   0x10
#define LOCATION_REMOVED 0x20
#define LOCATION_EXTRA   0x40
#define LOCATION_OVERLAY 0x80
#define LOCATION_ONFIELD 0x0C
#define LOCATION_FZONE   0x100
#define LOCATION_PZONE   0x200
#define LOCATION_ALL     0x3FF

#define MSG_RETRY                1
#define MSG_HINT                 2
#define MSG_WAITING              3
#define MSG_START                4
#define MSG_WIN                  5
#define MSG_UPDATE_DATA          6
#define MSG_UPDATE_CARD          7
// #define MSG_REQUEST_DECK         8
#define MSG_SELECT_BATTLECMD     10
#define MSG_SELECT_IDLECMD       11
#define MSG_SELECT_EFFECTYN      12
#define MSG_SELECT_YESNO         13
#define MSG_SELECT_OPTION        14
#define MSG_SELECT_CARD          15
#define MSG_SELECT_CHAIN         16
#define MSG_SELECT_PLACE         18
#define MSG_SELECT_POSITION      19
#define MSG_SELECT_TRIBUTE       20
#define MSG_SORT_CHAIN           21
#define MSG_SELECT_COUNTER       22
#define MSG_SELECT_SUM           23
#define MSG_SELECT_DISFIELD      24
#define MSG_SORT_CARD            25
#define MSG_SELECT_UNSELECT_CARD 26
#define MSG_CONFIRM_DECKTOP      30
#define MSG_CONFIRM_CARDS        31
#define MSG_SHUFFLE_DECK         32
#define MSG_SHUFFLE_HAND         33
// #define MSG_REFRESH_DECK         34
#define MSG_SWAP_GRAVE_DECK      35
#define MSG_SHUFFLE_SET_CARD     36
#define MSG_REVERSE_DECK         37
#define MSG_DECK_TOP             38
#define MSG_SHUFFLE_EXTRA        39
#define MSG_NEW_TURN             40
#define MSG_NEW_PHASE            41
#define MSG_CONFIRM_EXTRATOP     42
#define MSG_MOVE                 50
#define MSG_POS_CHANGE           53
#define MSG_SET                  54
#define MSG_SWAP                 55
#define MSG_FIELD_DISABLED       56
#define MSG_SUMMONING            60
#define MSG_SUMMONED             61
#define MSG_SPSUMMONING          62
#define MSG_SPSUMMONED           63
#define MSG_FLIPSUMMONING        64
#define MSG_FLIPSUMMONED         65
#define MSG_CHAINING             70
#define MSG_CHAINED              71
#define MSG_CHAIN_SOLVING        72
#define MSG_CHAIN_SOLVED         73
#define MSG_CHAIN_END            74
#define MSG_CHAIN_NEGATED        75
#define MSG_CHAIN_DISABLED       76
// #define MSG_CARD_SELECTED        80
#define MSG_RANDOM_SELECTED      81
#define MSG_BECOME_TARGET        83
#define MSG_DRAW                 90
#define MSG_DAMAGE               91
#define MSG_RECOVER              92
#define MSG_EQUIP                93
#define MSG_LPUPDATE             94
// #define MSG_UNEQUIP              95
#define MSG_CARD_TARGET          96
#define MSG_CANCEL_TARGET        97
#define MSG_PAY_LPCOST           100
#define MSG_ADD_COUNTER          101
#define MSG_REMOVE_COUNTER       102
#define MSG_ATTACK               110
#define MSG_BATTLE               111
#define MSG_ATTACK_DISABLED      112
#define MSG_DAMAGE_STEP_START    113
#define MSG_DAMAGE_STEP_END      114
#define MSG_MISSED_EFFECT        120
// #define MSG_BE_CHAIN_TARGET      121
// #define MSG_CREATE_RELATION      122
// #define MSG_RELEASE_RELATION     123
#define MSG_TOSS_COIN            130
#define MSG_TOSS_DICE            131
#define MSG_ROCK_PAPER_SCISSORS  132
#define MSG_HAND_RES             133
#define MSG_ANNOUNCE_RACE        140
#define MSG_ANNOUNCE_ATTRIB      141
#define MSG_ANNOUNCE_CARD        142
#define MSG_ANNOUNCE_NUMBER      143
#define MSG_ANNOUNCE_CARD_FILTER 144
#define MSG_CARD_HINT            160
#define MSG_TAG_SWAP             161
#define MSG_RELOAD_FIELD         162
// #define MSG_AI_NAME              163
// #define MSG_SHOW_HINT            164
#define MSG_PLAYER_HINT          165
#define MSG_MATCH_KILL           170
// #define MSG_CUSTOM_MSG   180

#define POS_FACEUP_ATTACK    0x1
#define POS_FACEDOWN_ATTACK  0x2
#define POS_FACEUP_DEFENSE   0x4
#define POS_FACEDOWN_DEFENSE 0x8
#define POS_FACEUP           (POS_FACEUP_ATTACK + POS_FACEUP_DEFENSE)
#define POS_FACEDOWN         (POS_FACEDOWN_ATTACK + POS_FACEDOWN_DEFENSE)
#define POS_ATTACK           (POS_FACEUP_ATTACK + POS_FACEDOWN_ATTACK)
#define POS_DEFENSE          (POS_FACEUP_DEFENSE + POS_FACEDOWN_DEFENSE)

#define SCOPE_OCG        0x1
#define SCOPE_TCG        0x2
#define SCOPE_ANIME      0x4
#define SCOPE_ILLEGAL    0x8
#define SCOPE_VIDEO_GAME 0x10
#define SCOPE_CUSTOM     0x20
#define SCOPE_SPEED      0x40
#define SCOPE_PRERELEASE 0x100
#define SCOPE_RUSH       0x200
#define SCOPE_LEGEND     0x400
#define SCOPE_HIDDEN     0x1000
#define SCOPE_OCG_TCG    (SCOPE_OCG | SCOPE_TCG)
#define SCOPE_OFFICIAL   (SCOPE_OCG | SCOPE_TCG | SCOPE_PRERELEASE)

#define QUERY_CODE         0x1
#define QUERY_POSITION     0x2
#define QUERY_ALIAS        0x4
#define QUERY_TYPE         0x8
#define QUERY_LEVEL        0x10
#define QUERY_RANK         0x20
#define QUERY_ATTRIBUTE    0x40
#define QUERY_RACE         0x80
#define QUERY_ATTACK       0x100
#define QUERY_DEFENSE      0x200
#define QUERY_BASE_ATTACK  0x400
#define QUERY_BASE_DEFENSE 0x800
#define QUERY_REASON       0x1000
#define QUERY_REASON_CARD  0x2000
#define QUERY_EQUIP_CARD   0x4000
#define QUERY_TARGET_CARD  0x8000
#define QUERY_OVERLAY_CARD 0x10000
#define QUERY_COUNTERS     0x20000
#define QUERY_OWNER        0x40000
#define QUERY_STATUS       0x80000
#define QUERY_IS_PUBLIC    0x100000
#define QUERY_LSCALE       0x200000
#define QUERY_RSCALE       0x400000
#define QUERY_LINK         0x800000
#define QUERY_IS_HIDDEN    0x1000000
#define QUERY_COVER        0x2000000
#define QUERY_END          0x80000000

#define TYPE_MONSTER     0x1
#define TYPE_SPELL       0x2
#define TYPE_TRAP        0x4
#define TYPE_NORMAL      0x10
#define TYPE_EFFECT      0x20
#define TYPE_FUSION      0x40
#define TYPE_RITUAL      0x80
#define TYPE_TRAPMONSTER 0x100
#define TYPE_SPIRIT      0x200
#define TYPE_UNION       0x400
#define TYPE_GEMINI      0x800
#define TYPE_TUNER       0x1000
#define TYPE_SYNCHRO     0x2000
#define TYPE_TOKEN       0x4000
#define TYPE_QUICKPLAY   0x10000
#define TYPE_CONTINUOUS  0x20000
#define TYPE_EQUIP       0x40000
#define TYPE_FIELD       0x80000
#define TYPE_COUNTER     0x100000
#define TYPE_FLIP        0x200000
#define TYPE_TOON        0x400000
#define TYPE_XYZ         0x800000
#define TYPE_PENDULUM    0x1000000
#define TYPE_SPSUMMON    0x2000000
#define TYPE_LINK        0x4000000
#define TYPE_SKILL       0x8000000

#define WIN_REASON_SURRENDERED     0x00
#define WIN_REASON_TIMED_OUT       0x03
#define WIN_REASON_CONNECTION_LOST 0x04
#define WIN_REASON_WRONG_RESPONSE  0x05
#define WIN_REASON_INTERNAL_ERROR  0x06

#define DUEL_RELAY 0x80

#endif // YGOPRO_CONSTANTS_HPP
