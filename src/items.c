/* items.c -- auto-generated from items.py */
#include "items.h"
#include "mm_util.h"

static ItemDef s_items[] = {
/*  id  name                       cost   slot  ac  dmg cls spell */
    {  0, "Empty                     ",      0, 0,   0,   0, 0xFF,   0},
    {  1, "Club                      ",      1, 1,   0,   0, 0xFF,   0},
    {  2, "Dagger                    ",      5, 1,   0,   0, 0x37,   0},
    {  3, "Hand Axe                  ",     10, 1,   0,   0, 0x27,   0},
    {  4, "Spear                     ",     15, 1,   0,   0, 0x07,   0},
    {  5, "Short Sword               ",     20, 1,   0,   0, 0x27,   0},
    {  6, "Mace                      ",     40, 1,   0,   0, 0x2E,   0},
    {  7, "Flail                     ",     40, 1,   0,   0, 0x2E,   0},
    {  8, "Scimitar                  ",     40, 1,   0,   0, 0x27,   0},
    {  9, "Broad Sword               ",     50, 1,   0,   0, 0x27,   0},
    { 10, "Battle Axe                ",     60, 1,   0,   0, 0x27,   0},
    { 11, "Long Sword                ",     60, 1,   0,   0, 0x27,   0},
    { 12, "Club +1                   ",     30, 1,   0,   1, 0xFF,   0},
    { 13, "Club +2                   ",    100, 1,   0,   2, 0xFF,   0},
    { 14, "Dagger +1                 ",     50, 1,   0,   1, 0x37,   0},
    { 15, "Hand Axe +1               ",     75, 1,   0,   1, 0x27,   0},
    { 16, "Spear +1                  ",    100, 1,   0,   1, 0x07,   0},
    { 17, "Short Sword +1            ",    100, 1,   0,   1, 0x27,   0},
    { 18, "Mace +1                   ",    125, 1,   0,   1, 0x2E,   0},
    { 19, "Flail +1                  ",    200, 1,   0,   1, 0x2E,   0},
    { 20, "Scimitar +1               ",    250, 1,   0,   1, 0x27,   0},
    { 21, "Broad Sword +1            ",    300, 1,   0,   1, 0x27,   0},
    { 22, "Battle Axe +1             ",    300, 1,   0,   1, 0x27,   0},
    { 23, "Long Sword +1             ",    300, 1,   0,   1, 0x27,   0},
    { 24, "Flaming Club              ",    500, 1,   0,   3, 0xFF,  50},
    { 25, "Club Of Noise             ",    100, 1,   0,   0, 0xFF,   0},
    { 26, "Dagger +2                 ",    200, 1,   0,   2, 0x37,  52},
    { 27, "Hand Axe +2               ",    225, 1,   0,   2, 0x27,   0},
    { 28, "Spear +2                  ",    250, 1,   0,   2, 0x07,   0},
    { 29, "Short Sword +2            ",    300, 1,   0,   2, 0x27,  48},
    { 30, "Mace +2                   ",    325, 1,   0,   2, 0x2E,   7},
    { 31, "Flail +2                  ",    350, 1,   0,   2, 0x2E,   3},
    { 32, "Scimitar +2               ",    400, 1,   0,   2, 0x27,   0},
    { 33, "Broad Sword +2            ",    400, 1,   0,   2, 0x27,   0},
    { 34, "Battle Axe +2             ",    500, 1,   0,   2, 0x27,   0},
    { 35, "Long Sword +2             ",    550, 1,   0,   2, 0x27,   0},
    { 36, "Royal Dagger              ",   2500, 1,   0,   0, 0x37,   0},
    { 37, "Dagger Of Mind            ",    750, 1,   0,   3, 0x10,  77},
    { 38, "Diamond Dagger            ",    800, 1,   0,   4, 0x10,   0},
    { 39, "Electric Spear            ",   1200, 1,   0,   3, 0x07,  55},
    { 40, "Holy Mace                 ",   2000, 1,   0,   4, 0x08,  38},
    { 41, "Un-Holy Mace              ",   2000, 1,   0,   4, 0x08,  37},
    { 42, "Dark Flail                ",    600, 1,   0,   0, 0x2E,  33},
    { 43, "Flail Of Fear             ",   1600, 1,   0,   3, 0x08,  62},
    { 44, "Lucky Scimitar            ",   2200, 1,   0,   4, 0x27,   0},
    { 45, "Mace Of Undead            ",    500, 1,   0,   0, 0x2E,   0},
    { 46, "Cold Axe                  ",   2500, 1,   0,   3, 0x21,  72},
    { 47, "Electric Sword            ",   2200, 1,   0,   3, 0x07,  66},
    { 48, "Flaming Sword             ",   2200, 1,   0,   3, 0x07,  63},
    { 49, "Sword Of Might            ",   8000, 1,   0,   5, 0x20,   0},
    { 50, "Sword Of Speed            ",   7000, 1,   0,   5, 0x07,   0},
    { 51, "Sharp Sword               ",   6500, 1,   0,   4, 0x21,  81},
    { 52, "Accurate Sword            ",   6500, 1,   0,   6, 0x07,   0},
    { 53, "Sword Of Magic            ",  10000, 1,   0,   5, 0x27,  87},
    { 54, "Immortal Sword            ",   7000, 1,   0,   4, 0x27,  39},
    { 55, "Axe Protector             ",   8000, 1,   0,   5, 0x07,  92},
    { 56, "Axe Destroyer             ",   8000, 1,   0,   5, 0x21,  85},
    { 57, "X!Xx!X'S Sword            ",   6000, 1,   0,   4, 0x27,   0},
    { 58, "Adamantine Axe            ",  12000, 1,   0,   5, 0x07,  36},
    { 59, "Ultimate Sword            ",  15000, 1,   0,   6, 0x27,   0},
    { 60, "Element Sword             ",  12000, 1,   0,   5, 0x27,  44},
    { 61, "Sling                     ",     10, 2,   0,   0, 0x27,   0},
    { 62, "Crossbow                  ",     50, 2,   0,   0, 0x27,   0},
    { 63, "Short Bow                 ",     75, 2,   0,   0, 0x07,   0},
    { 64, "Long Bow                  ",    100, 2,   0,   0, 0x07,   0},
    { 65, "Great Bow                 ",    250, 2,   0,   0, 0x07,   0},
    { 66, "Sling +1                  ",     50, 2,   0,   1, 0x27,   0},
    { 67, "Crossbow +1               ",    250, 2,   0,   1, 0x27,   0},
    { 68, "Short Bow +1              ",    375, 2,   0,   1, 0x07,   0},
    { 69, "Long Bow +1               ",    500, 2,   0,   1, 0x07,   0},
    { 70, "Great Bow +1              ",   1250, 2,   0,   1, 0x07,   0},
    { 71, "Magic Sling               ",    800, 2,   0,   3, 0x27,   0},
    { 72, "Crossbow +2               ",   1000, 2,   0,   2, 0x17,   0},
    { 73, "Short Bow +2              ",   1000, 2,   0,   2, 0x07,   0},
    { 74, "Long Bow +2               ",   1200, 2,   0,   2, 0x07,   0},
    { 75, "Great Bow +2              ",   2000, 2,   0,   2, 0x07,   0},
    { 76, "Crossbow Luck             ",   2000, 2,   0,   3, 0x17,   1},
    { 77, "Crossbow Speed            ",   2000, 2,   0,   3, 0x27,   2},
    { 78, "Lightning Bow             ",   3000, 2,   0,   3, 0x07,  63},
    { 79, "Flaming Bow               ",   3000, 2,   0,   3, 0x07,  66},
    { 80, "Giant'S Bow               ",   2000, 2,   0,   3, 0x07,   0},
    { 81, "The Magic Bow             ",   6000, 2,   0,   4, 0x07,  83},
    { 82, "Bow Of Power              ",   6000, 2,   0,   4, 0x07,   0},
    { 83, "Robber'S X-Bow            ",   8000, 2,   0,   5, 0x20,  61},
    { 84, "Archer'S Bow              ",  12000, 2,   0,   5, 0x08,  85},
    { 85, "Obsidian Bow              ",   2000, 2,   0,   0, 0xFF,  80},
    { 86, "Staff                     ",     30, 3,   0,   0, 0x1E,   0},
    { 87, "Glaive                    ",     80, 3,   0,   0, 0x07,   0},
    { 88, "Bardiche                  ",     80, 3,   0,   0, 0x07,   0},
    { 89, "Halberd                   ",    100, 3,   0,   0, 0x07,   0},
    { 90, "Great Hammer              ",    150, 3,   0,   0, 0x06,   0},
    { 91, "Great Axe                 ",    150, 3,   0,   0, 0x07,   0},
    { 92, "Flamberge                 ",    250, 3,   0,   0, 0x07,   0},
    { 93, "Staff +1                  ",    200, 3,   0,   1, 0x1E,   0},
    { 94, "Glaive +1                 ",    350, 3,   0,   1, 0x07,   0},
    { 95, "Bardiche +1               ",    350, 3,   0,   1, 0x07,   0},
    { 96, "Halberd +1                ",    500, 3,   0,   1, 0x07,   0},
    { 97, "Great Hammer+1            ",    550, 3,   0,   1, 0x06,   0},
    { 98, "Great Axe +1              ",    500, 3,   0,   1, 0x07,   0},
    { 99, "Flamberge +1              ",    600, 3,   0,   1, 0x07,   0},
    {100, "Staff +2                  ",    600, 3,   0,   2, 0x1E,  54},
    {101, "Glaive +2                 ",    900, 3,   0,   2, 0x07,   0},
    {102, "Bardiche +2               ",    900, 3,   0,   2, 0x07,   0},
    {103, "Halberd +2                ",   1200, 3,   0,   2, 0x07,   3},
    {104, "Great Hammer+2            ",   1200, 3,   0,   2, 0x06,   1},
    {105, "Great Axe +2              ",   1200, 3,   0,   2, 0x07,   0},
    {106, "Flamberge +2              ",   2000, 3,   0,   2, 0x07,   0},
    {107, "Staff Of Light            ",   1500, 3,   0,   3, 0x1E,  19},
    {108, "Cold Glaive               ",   2500, 3,   0,   3, 0x07,  21},
    {109, "Curing Staff              ",   2500, 3,   0,   3, 0x0C,   5},
    {110, "Minotaur'S Axe            ",   2000, 3,   0,   0, 0x07,   0},
    {111, "Thunder Hammer            ",   3500, 3,   0,   4, 0x08,  29},
    {112, "Great Axe +3              ",   3500, 3,   0,   3, 0x07,   0},
    {113, "Flamberge +3              ",   5000, 3,   0,   3, 0x07,   0},
    {114, "Sorcerer Staff            ",   8000, 3,   0,   5, 0xFF,  91},
    {115, "Staff Of Magic            ",   5000, 3,   0,   4, 0x1E,  87},
    {116, "Demon'S Glaive            ",  10000, 3,   0,   5, 0x15,  71},
    {117, "Devil'S Glaive            ",  10000, 3,   0,   5, 0x15,  72},
    {118, "The Flamberge             ",  15000, 3,   0,   6, 0x07,  73},
    {119, "Holy Flamberge            ",  20000, 3,   0,   6, 0x01,  43},
    {120, "Evil Flamberge            ",  20000, 3,   0,   6, 0x01,  46},
    {121, "Padded Armor              ",     10, 4,   1,   0, 0xFF,   0},
    {122, "Leather Armor             ",     20, 4,   2,   0, 0x2F,   0},
    {123, "Scale Armor               ",     50, 4,   3,   0, 0x2F,   0},
    {124, "Ring Mail                 ",    100, 4,   4,   0, 0x2F,   0},
    {125, "Chain Mail                ",    200, 4,   5,   0, 0x2D,   0},
    {126, "Splint Mail               ",    400, 4,   6,   0, 0x21,   0},
    {127, "Plate Mail                ",   1000, 4,   7,   0, 0x21,   0},
    {128, "Padded +1                 ",     25, 4,   2,   0, 0xFF,   0},
    {129, "Leather +1                ",     60, 4,   3,   0, 0x2F,   0},
    {130, "Scale +1                  ",    120, 4,   4,   0, 0x2F,   0},
    {131, "Ring Mail +1              ",    250, 4,   5,   0, 0x2F,   0},
    {132, "Chain Mail +1             ",    500, 4,   6,   0, 0x2D,   0},
    {133, "Splint Mail +1            ",   1000, 4,   7,   0, 0x21,   0},
    {134, "Plate Mail +1             ",   2500, 4,   8,   0, 0x21,   0},
    {135, "Leather +2                ",    150, 4,   4,   0, 0x2F,   0},
    {136, "Scale +2                  ",    300, 4,   5,   0, 0x2F,   0},
    {137, "Ring Mail +2              ",    750, 4,   6,   0, 0x2F,   0},
    {138, "Chain Mail +2             ",   1500, 4,   7,   0, 0x2D,   0},
    {139, "Splint Mail +2            ",   2500, 4,   8,   0, 0x21,   0},
    {140, "Plate Mail +2             ",   7500, 4,   9,   0, 0x21,   0},
    {141, "Bracers Ac 4              ",   1000, 4,   4,   0, 0x17,   0},
    {142, "Ring Mail +3              ",   2000, 4,   7,   0, 0x2F,   0},
    {143, "Chain Mail +3             ",   4500, 4,   8,   0, 0x2D,   0},
    {144, "Splint Mail +3            ",   7500, 4,   9,   0, 0x21,   0},
    {145, "Plate Mail +3             ",  15000, 4,  10,   0, 0x21,   0},
    {146, "Bracers Ac 6              ",   2500, 4,   6,   0, 0x17,  77},
    {147, "Chain Mail +3             ",   4500, 4,   0,   0, 0x2D,   0},
    {148, "Bracers Ac 8              ",   7500, 4,   0,   0, 0xFF,   0},
    {149, "Blue Ring Mail            ",  10000, 4,   9,   0, 0x2F,  66},
    {150, "Red Chain Mail            ",  15000, 4,  10,   0, 0x2D,  63},
    {151, "X!Xx!X'S Plate            ",  18000, 4,  11,   0, 0x21,   0},
    {152, "Holy Plate                ",  25000, 4,  12,   0, 0x01,   0},
    {153, "Un-Holy Plate             ",  25000, 4,  12,   0, 0x01,   0},
    {154, "Ultimate Plate            ",  30000, 4,  13,   0, 0x25,  49},
    {155, "Bracers Ac 8              ",   7500, 4,   8,   0, 0x17,  77},
    {156, "Small Shield              ",     10, 5,   1,   0, 0x2C,   0},
    {157, "Large Shield              ",     50, 5,   2,   0, 0x2C,   0},
    {158, "Silver Shield             ",    100, 5,   2,   0, 0x2C,   0},
    {159, "Small Shield+1            ",    100, 5,   2,   0, 0x2C,   0},
    {160, "Large Shield+1            ",    200, 5,   3,   0, 0x2C,   0},
    {161, "Large Shield+1            ",    200, 5,   0,   0, 0x2C,   0},
    {162, "Small Shield+2            ",    400, 5,   3,   0, 0x2C,   0},
    {163, "Large Shield+2            ",    800, 5,   4,   0, 0x2C,   0},
    {164, "Large Shield+2            ",    800, 5,   0,   0, 0x2C,   0},
    {165, "Fire Shield               ",   2500, 5,   5,   0, 0x2C,   0},
    {166, "Cold Shield               ",   2500, 5,   5,   0, 0x2C,   0},
    {167, "Elec Shield               ",   2500, 5,   5,   0, 0x2C,   0},
    {168, "Acid Shield               ",   2500, 5,   5,   0, 0x2C,   0},
    {169, "Magic Shield              ",   5000, 5,   6,   0, 0x2C,  77},
    {170, "Dragon Shield             ",   8000, 5,   7,   0, 0x2C,  92},
    {171, "Rope & Hooks              ",     10, 0,   0,   0, 0xFF,  58},
    {172, "Torch                     ",      2, 0,   0,   0, 0xFF,   4},
    {173, "Lantern                   ",     20, 0,   0,   0, 0xFF,   4},
    {174, "10 Foot Pole              ",     10, 0,   0,   0, 0xFF,   0},
    {175, "Garlic                    ",      5, 0,   0,   0, 0xFF,   0},
    {176, "Wolfsbane                 ",     10, 0,   0,   0, 0xFF,   0},
    {177, "Belladonna                ",     25, 0,   0,   0, 0xFF,   0},
    {178, "Magic Herbs               ",     50, 0,   0,   0, 0xFF,   3},
    {179, "Dried Beef                ",     40, 0,   0,   0, 0xFF,   0},
    {180, "Robber'S Tools            ",    150, 0,   0,   0, 0x20,   0},
    {181, "Bag Of Silver             ",    300, 0,   0,   0, 0xFF,   0},
    {182, "Amber Gem                 ",    500, 0,   0,   0, 0xFF,   0},
    {183, "Smelling Salt             ",     50, 0,   0,   0, 0xFF,   0},
    {184, "Bag Of Sand               ",    100, 0,   0,   0, 0xFF,  54},
    {185, "Might Potion              ",    200, 0,   0,   0, 0xFF,   0},
    {186, "Speed Potion              ",    200, 0,   0,   0, 0xFF,   0},
    {187, "Sundial                   ",    500, 0,   0,   0, 0xFF,  53},
    {188, "Curing Potion             ",    350, 0,   0,   0, 0xFF,   8},
    {189, "Magic Potion              ",    500, 0,   0,   0, 0xFF,   0},
    {190, "Defense Ring              ",    500, 0,   0,   0, 0xFF,  57},
    {191, "Bag Of Garbage            ",    100, 0,   0,   0, 0xFF,   0},
    {192, "Scroll Of Fire            ",    300, 0,   0,   0, 0xFF,  63},
    {193, "Flying Carpet             ",    500, 0,   0,   0, 0x10,  64},
    {194, "Jade Amulet               ",    600, 0,   0,   0, 0xFF,   0},
    {195, "Antidote Brew             ",    500, 0,   0,   0, 0xFF,  25},
    {196, "Skill Potion              ",    600, 0,   0,   0, 0xFF,   0},
    {197, "Boots Of Speed            ",    800, 0,   0,   0, 0xFF,   0},
    {198, "Lucky Charm               ",    800, 0,   0,   0, 0xFF,   0},
    {199, "Wand Of Fire              ",   1000, 0,   0,   0, 0x17,  63},
    {200, "Undead Amulet             ",    800, 0,   0,   0, 0xFF,   7},
    {201, "Silent Chime              ",    400, 0,   0,   0, 0xFF,  14},
    {202, "Belt Of Power             ",    600, 0,   0,   0, 0x25,   0},
    {203, "Model Boat                ",    400, 0,   0,   0, 0xFF,  23},
    {204, "Defense Cloak             ",    700, 0,   0,   0, 0xFF,   0},
    {205, "Knowledge Book            ",   1000, 0,   0,   0, 0x29,   0},
    {206, "Ruby Idol                 ",   3000, 0,   0,   0, 0xFF,   0},
    {207, "Sorcerer Robe             ",   2500, 0,   0,   0, 0x10,  65},
    {208, "Power Gauntlet            ",   3000, 0,   0,   0, 0x2E,   0},
    {209, "Cleric'S Beads            ",   3000, 0,   0,   0, 0x08,   8},
    {210, "Horn Of Death             ",   2500, 0,   0,   0, 0xFF,  81},
    {211, "Potion Of Life            ",   1500, 0,   0,   0, 0xFF,  38},
    {212, "Shiny Pendant             ",   2000, 0,   0,   0, 0xFF,  56},
    {213, "Lightning Wand            ",   1500, 0,   0,   0, 0x17,  66},
    {214, "Precision Ring            ",   3000, 0,   0,   0, 0xFF,   0},
    {215, "Return Scroll             ",   2000, 0,   0,   0, 0xFF,  41},
    {216, "Teleport Helm             ",   5000, 0,   0,   0, 0xFF,  83},
    {217, "Youth Potion              ",   4000, 0,   0,   0, 0xFF,  39},
    {218, "Bells Of Time             ",   1000, 0,   0,   0, 0xFF,   0},
    {219, "Magic Oil                 ",   3000, 0,   0,   0, 0xFF,  88},
    {220, "Magic Vest                ",   6000, 0,   0,   0, 0xFF,  78},
    {221, "Destroyer Wand            ",   7000, 0,   0,   0, 0x1A,  85},
    {222, "Element Scarab            ",   6000, 0,   0,   0, 0xFF,  44},
    {223, "Sun Scroll                ",   3000, 0,   0,   0, 0xFF,  46},
    {224, "Star Ruby                 ",   6000, 0,   0,   0, 0xFF,  49},
    {225, "Star Sapphire             ",   6000, 0,   0,   0, 0xFF,  87},
    {226, "Wealth Chest              ",   6000, 0,   0,   0, 0xFF,   0},
    {227, "Gem Sack                  ",  10000, 0,   0,   0, 0xFF,   0},
    {228, "Diamond Collar            ",  10000, 0,   0,   0, 0xFF,  93},
    {229, "Fire Opal                 ",  10000, 0,   0,   0, 0xFF,  91},
    {230, "Unobtainium               ",  50000, 0,   0,   0, 0xFF,   0},
    {231, "Vellum Scroll             ",     10, 0,   0,   0, 0xFF,   0},
    {232, "Ruby Whistle              ",    500, 0,   0,   0, 0xFF,   0},
    {233, "Kings Pass                ",      0, 0,   0,   0, 0xFF,   0},
    {234, "Merchants Pass            ",      0, 0,   0,   0, 0xFF,   0},
    {235, "Crystal Key               ",   1000, 0,   0,   0, 0xFF,  93},
    {236, "Coral Key                 ",    300, 0,   0,   0, 0xFF,  23},
    {237, "Bronze Key                ",    500, 0,   0,   0, 0xFF,  48},
    {238, "Silver Key                ",    600, 0,   0,   0, 0xFF,  51},
    {239, "Gold Key                  ",    800, 0,   0,   0, 0xFF,  65},
    {240, "Diamond Key               ",   2000, 0,   0,   0, 0xFF,  83},
    {241, "Cactus Nectar             ",    400, 0,   0,   0, 0xFF,  16},
    {242, "Map Of Desert             ",    400, 0,   0,   0, 0xFF,  53},
    {243, "Laser Blaster             ",   2000, 0,   0,   0, 0xFF,  85},
    {244, "Dragons Tooth             ",   1500, 0,   0,   0, 0xFF,  39},
    {245, "Wyvern Eye                ",   1000, 0,   0,   0, 0xFF,  62},
    {246, "Medusa Head               ",      0, 0,   0,   0, 0xFF,   0},
    {247, "Ring Of Okrim             ",   3000, 0,   0,   0, 0xFF,  78},
    {248, "B Queen Idol              ",      0, 0,   0,   0, 0xFF,   0},
    {249, "W Queen Idol              ",      0, 0,   0,   0, 0xFF,   0},
    {250, "Pirates Map A             ",   1000, 0,   0,   0, 0xFF,   0},
    {251, "Pirates Map B             ",   2000, 0,   0,   0, 0xFF,   0},
    {252, "Thundranium               ",  10000, 0,   0,   0, 0xFF,   0},
    {253, "Key Card                  ",      0, 0,   0,   0, 0xFF,   0},
    {254, "Eye Of Goros              ",  10000, 0,   0,   0, 0xFF,  89},
    {255, "(Useless Item)            ",      0, 0,   0,   0, 0xFF,   0},
};
#define NUM_ITEMS 256

const ItemDef *item_get(int id) {
    for (int i = 0; i < NUM_ITEMS; i++) if (s_items[i].id == id) return &s_items[i];
    return 0;
}

/* ---------------------------------------------------------------------------
 * Item extras: alignment restrictions + stat/resistance bonuses when equipped.
 * stat_idx: 1=INT 2=MIG 3=PER 4=END 5=SPD 6=ACC 7=LCK
 * res_idx:  1=magic 2=fire 3=cold 4=elec 5=acid 6=fear 7=poison 8=sleep
 * -------------------------------------------------------------------------- */
static const ItemExtras s_item_extras[] = {
    /* id  align         stat  sval  res  rval */
    { 40, ALIGN_GOOD,   0,    0,    0,   0   },  /* Holy Mace             */
    { 41, ALIGN_EVIL,   0,    0,    0,   0   },  /* Un-Holy Mace          */
    { 43, ALIGN_EVIL,   0,    0,    6,  20   },  /* Flail Of Fear  +20% fear res */
    { 37, ALIGN_ANY,    1,    2,    0,   0   },  /* Dagger Of Mind +2 INT */
    { 44, ALIGN_ANY,    7,    2,    0,   0   },  /* Lucky Scimitar +2 LCK */
    { 49, ALIGN_ANY,    2,    3,    0,   0   },  /* Sword Of Might +3 MIG */
    { 50, ALIGN_ANY,    5,    3,    0,   0   },  /* Sword Of Speed +3 SPD */
    { 52, ALIGN_ANY,    6,    3,    0,   0   },  /* Accurate Sword +3 ACC */
    { 53, ALIGN_ANY,    1,    2,    0,   0   },  /* Sword Of Magic +2 INT */
    { 54, ALIGN_ANY,    4,    2,    0,   0   },  /* Immortal Sword +2 END */
    { 36, ALIGN_ANY,    3,    2,    0,   0   },  /* Royal Dagger   +2 PER */
    { 24, ALIGN_ANY,    0,    0,    2,  25   },  /* Flaming Club  +25% fire res */
    { 46, ALIGN_ANY,    0,    0,    3,  25   },  /* Cold Axe      +25% cold res */
    { 47, ALIGN_ANY,    0,    0,    4,  25   },  /* Electric Sword+25% elec res */
    { 48, ALIGN_ANY,    0,    0,    2,  25   },  /* Flaming Sword +25% fire res */
};
#define NUM_ITEM_EXTRAS ((int)(sizeof(s_item_extras)/sizeof(s_item_extras[0])))

static const ItemExtras s_zero_extras = { 0, ALIGN_ANY, 0, 0, 0, 0 };

const ItemExtras *item_get_extras(int id) {
    for (int i = 0; i < NUM_ITEM_EXTRAS; i++)
        if (s_item_extras[i].id == id) return &s_item_extras[i];
    return &s_zero_extras;
}

const char *item_name(int id) { const ItemDef *d = item_get(id); return d ? d->name : "?"; }

static const int s_sorpigal[] = { 1, 2, 3, 4, 5, 6, 61, 62, 86, 121, 122, 123, 156, 157, 171, 172, 173, 174, 175, 176, 177, 178, 185, 186 };
#define S_SORPIGAL_N 24
static const int s_portsmith[] = { 5, 6, 7, 8, 9, 62, 63, 86, 87, 122, 123, 124, 156, 157, 158, 172, 173, 175, 178, 185, 186, 188 };
#define S_PORTSMITH_N 22
static const int s_algary[] = { 9, 10, 11, 17, 18, 62, 63, 64, 87, 88, 123, 124, 125, 157, 159, 160, 173, 178, 184, 185, 186, 187, 188 };
#define S_ALGARY_N 23
static const int s_dusk[] = { 17, 18, 22, 23, 62, 63, 64, 95, 96, 124, 125, 126, 159, 160, 162, 178, 185, 186, 187, 188, 189, 192 };
#define S_DUSK_N 22
static const int s_erliquin[] = { 22, 23, 34, 35, 64, 65, 96, 99, 125, 126, 127, 160, 162, 163, 185, 186, 187, 188, 189, 192, 193 };
#define S_ERLIQUIN_N 21

static const int *s_stocks[5]  = {s_sorpigal,s_portsmith,s_algary,s_dusk,s_erliquin};
static const int  s_stock_n[5] = {S_SORPIGAL_N,S_PORTSMITH_N,S_ALGARY_N,S_DUSK_N,S_ERLIQUIN_N};

const int *shop_stock(int town, int *count) {
    if (town<0||town>4){*count=0;return 0;}
    *count = s_stock_n[town]; return s_stocks[town];
}
