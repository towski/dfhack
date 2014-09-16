// Dwarf Manipulator - a Therapist-style labor editor

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <MiscUtils.h>
#include <modules/Screen.h>
#include <modules/Translation.h>
#include <modules/Units.h>
#include <vector>
#include <string>
#include <set>
#include <algorithm>

#include <VTableInterpose.h>
#include "df/world.h"
#include "df/ui.h"
#include "df/graphic.h"
#include "df/enabler.h"
#include "df/viewscreen_unitlistst.h"
#include "df/interface_key.h"
#include "df/unit.h"
#include "df/squad.h"
#include "df/squad_position.h"
#include "df/job.h"
#include "df/unit_soul.h"
#include "df/unit_skill.h"
#include "df/physical_attribute_type.h"
#include "df/creature_graphics_role.h"
#include "df/creature_raw.h"
#include "df/caste_raw.h"
#include "df/historical_entity.h"
#include "df/entity_raw.h"

using std::set;
using std::vector;
using std::string;

using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::ui;
using df::global::gps;
using df::global::enabler;

struct SkillLevel
{
    const char *name;
    int points;
    char abbrev;
};

#define NUM_SKILL_LEVELS (sizeof(skill_levels) / sizeof(SkillLevel))

// The various skill rankings. Zero skill is hardcoded to "Not" and '-'.
const SkillLevel skill_levels[] = {
    {"Dabbling",     500, '0'},
    {"Novice",       600, '1'},
    {"Adequate",     700, '2'},
    {"Competent",    800, '3'},
    {"Skilled",      900, '4'},
    {"Proficient",  1000, '5'},
    {"Talented",    1100, '6'},
    {"Adept",       1200, '7'},
    {"Expert",      1300, '8'},
    {"Professional",1400, '9'},
    {"Accomplished",1500, 'A'},
    {"Great",       1600, 'B'},
    {"Master",      1700, 'C'},
    {"High Master", 1800, 'D'},
    {"Grand Master",1900, 'E'},
    {"Legendary",   2000, 'U'},
    {"Legendary+1", 2100, 'V'},
    {"Legendary+2", 2200, 'W'},
    {"Legendary+3", 2300, 'X'},
    {"Legendary+4", 2400, 'Y'},
    {"Legendary+5",    0, 'Z'}
};

struct SkillColumn
{
    int group; // for navigation and mass toggling
    int8_t color; // for column headers
    df::profession profession; // to display graphical tiles
    df::unit_labor labor; // toggled when pressing Enter
    df::job_skill skill; // displayed rating
    df::physical_attribute_type attr; // displayed rating
    char label[3]; // column header
    bool special; // specified labor is mutually exclusive with all other special labors
    bool isValidLabor (df::historical_entity *entity = NULL) const
    {
        if (labor == unit_labor::NONE)
            return false;
        if (entity && entity->entity_raw && !entity->entity_raw->jobs.permitted_labor[labor])
            return false;
        return true;
    }
};

#define NUM_COLUMNS (sizeof(columns) / sizeof(SkillColumn))

// All of the skill/labor columns we want to display.
const SkillColumn columns[] = {
// Mining
    {1, 2, profession::NONE, unit_labor::NONE, job_skill::NONE, physical_attribute_type::STRENGTH, "St"},
    {1, 2, profession::NONE, unit_labor::NONE, job_skill::NONE, physical_attribute_type::AGILITY, "Ag"},
    {1, 2, profession::NONE, unit_labor::NONE, job_skill::NONE, physical_attribute_type::TOUGHNESS, "To"},
    {1, 2, profession::NONE, unit_labor::NONE, job_skill::NONE, physical_attribute_type::ENDURANCE, "En"},
    {1, 2, profession::NONE, unit_labor::NONE, job_skill::NONE, physical_attribute_type::RECUPERATION, "Re"},
    {1, 2, profession::NONE, unit_labor::NONE, job_skill::NONE, physical_attribute_type::DISEASE_RESISTANCE, "Di"},
// Military - Misc
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::DISCIPLINE, physical_attribute_type::STRENGTH, "Di"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::LEADERSHIP, physical_attribute_type::STRENGTH, "Ld"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::TEACHING, physical_attribute_type::STRENGTH, "Te"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::KNOWLEDGE_ACQUISITION, physical_attribute_type::STRENGTH, "St"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::CONCENTRATION, physical_attribute_type::STRENGTH, "Co"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::SITUATIONAL_AWARENESS, physical_attribute_type::STRENGTH, "Ob"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::COORDINATION, physical_attribute_type::STRENGTH, "Cr"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::BALANCE, physical_attribute_type::STRENGTH, "Ba"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::CLIMBING, physical_attribute_type::STRENGTH, "Cl"},
// Military - Other Combat
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::BITE, physical_attribute_type::STRENGTH, "Bi"},
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::GRASP_STRIKE, physical_attribute_type::STRENGTH, "St"},
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::STANCE_STRIKE, physical_attribute_type::STRENGTH, "Ki"},
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::MISC_WEAPON, physical_attribute_type::STRENGTH, "Mi"},
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::MELEE_COMBAT, physical_attribute_type::STRENGTH, "Fg"},
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::RANGED_COMBAT, physical_attribute_type::STRENGTH, "Ac"},
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::ARMOR, physical_attribute_type::STRENGTH, "Ar"},
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::SHIELD, physical_attribute_type::STRENGTH, "Sh"},
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::DODGING, physical_attribute_type::STRENGTH, "Do"},
// Military - Weapons
    {13, 7, profession::WRESTLER, unit_labor::NONE, job_skill::WRESTLING, physical_attribute_type::STRENGTH, "Wr"},
    {13, 7, profession::AXEMAN, unit_labor::NONE, job_skill::AXE, physical_attribute_type::STRENGTH, "Ax"},
    {13, 7, profession::SWORDSMAN, unit_labor::NONE, job_skill::SWORD, physical_attribute_type::STRENGTH, "Sw"},
    {13, 7, profession::MACEMAN, unit_labor::NONE, job_skill::MACE, physical_attribute_type::STRENGTH, "Mc"},
    {13, 7, profession::HAMMERMAN, unit_labor::NONE, job_skill::HAMMER, physical_attribute_type::STRENGTH, "Ha"},
    {13, 7, profession::SPEARMAN, unit_labor::NONE, job_skill::SPEAR, physical_attribute_type::STRENGTH, "Sp"},
    {13, 7, profession::CROSSBOWMAN, unit_labor::NONE, job_skill::CROSSBOW, physical_attribute_type::STRENGTH, "Cb"},
    {13, 7, profession::THIEF, unit_labor::NONE, job_skill::DAGGER, physical_attribute_type::STRENGTH, "Kn"},
    {13, 7, profession::BOWMAN, unit_labor::NONE, job_skill::BOW, physical_attribute_type::STRENGTH, "Bo"},
    {13, 7, profession::BLOWGUNMAN, unit_labor::NONE, job_skill::BLOWGUN, physical_attribute_type::STRENGTH, "Bl"},
    {13, 7, profession::PIKEMAN, unit_labor::NONE, job_skill::PIKE, physical_attribute_type::STRENGTH, "Pk"},
    {13, 7, profession::LASHER, unit_labor::NONE, job_skill::WHIP, physical_attribute_type::STRENGTH, "La"},
// Miscellaneous
    {18, 3, profession::NONE, unit_labor::NONE, job_skill::THROW, physical_attribute_type::STRENGTH, "Th"},
    {18, 3, profession::NONE, unit_labor::NONE, job_skill::CRUTCH_WALK, physical_attribute_type::STRENGTH, "CW"},
    {18, 3, profession::NONE, unit_labor::NONE, job_skill::SWIMMING, physical_attribute_type::STRENGTH, "Sw"},
    {18, 3, profession::NONE, unit_labor::NONE, job_skill::KNAPPING, physical_attribute_type::STRENGTH, "Kn"},
};

struct UnitInfo
{
    df::unit *unit;
    bool allowEdit;
    string name;
    string transname;
    string profession;
    int8_t color;
    int active_index;
    string squad_effective_name;
    string squad_info;
};

enum altsort_mode {
    ALTSORT_NAME,
    ALTSORT_PROFESSION_OR_SQUAD,
    ALTSORT_HAPPINESS,
    ALTSORT_ARRIVAL,
    ALTSORT_SKILL,
    ALTSORT_MAX
};

altsort_mode old_altsort = ALTSORT_MAX; 
int old_sel_column = 0;

bool descending;
df::job_skill sort_skill;
df::physical_attribute_type sort_attr; 
df::unit_labor sort_labor;

bool sortByName (const UnitInfo *d1, const UnitInfo *d2)
{
    if (descending)
        return (d1->name > d2->name);
    else
        return (d1->name < d2->name);
}

bool sortByProfession (const UnitInfo *d1, const UnitInfo *d2)
{
    if (descending)
        return (d1->profession > d2->profession);
    else
        return (d1->profession < d2->profession);
}

bool sortBySquad (const UnitInfo *d1, const UnitInfo *d2)
{
    bool gt = false;
    if (d1->unit->military.squad_id == -1 && d2->unit->military.squad_id == -1)
        gt = d1->name > d2->name;
    else if (d1->unit->military.squad_id == -1)
        gt = true;
    else if (d2->unit->military.squad_id == -1)
        gt = false;
    else if (d1->unit->military.squad_id != d2->unit->military.squad_id)
        gt = d1->squad_effective_name > d2->squad_effective_name;
    else
        gt = d1->unit->military.squad_position > d2->unit->military.squad_position;
    return descending ? gt : !gt;
}

bool sortByHappiness (const UnitInfo *d1, const UnitInfo *d2)
{
    if (descending)
        return (d1->unit->status.happiness > d2->unit->status.happiness);
    else
        return (d1->unit->status.happiness < d2->unit->status.happiness);
}

bool sortByArrival (const UnitInfo *d1, const UnitInfo *d2)
{
    if (descending)
        return (d1->active_index > d2->active_index);
    else
        return (d1->active_index < d2->active_index);
}

bool sortBySkill (const UnitInfo *d1, const UnitInfo *d2)
{
    if (sort_skill != job_skill::NONE)
    {
        if (!d1->unit->status.current_soul)
            return !descending;
        if (!d2->unit->status.current_soul)
            return descending;
        df::unit_skill *s1 = binsearch_in_vector<df::unit_skill,df::job_skill>(d1->unit->status.current_soul->skills, &df::unit_skill::id, sort_skill);
        df::unit_skill *s2 = binsearch_in_vector<df::unit_skill,df::job_skill>(d2->unit->status.current_soul->skills, &df::unit_skill::id, sort_skill);
        int l1 = s1 ? s1->rating : 0;
        int l2 = s2 ? s2->rating : 0;
        int e1 = s1 ? s1->experience : 0;
        int e2 = s2 ? s2->experience : 0;
        if (descending)
        {
            if (l1 != l2)
                return l1 > l2;
            if (e1 != e2)
                return e1 > e2;
        }
        else
        {
            if (l1 != l2)
                return l1 < l2;
            if (e1 != e2)
                return e1 < e2;
        }
    } else {
        int phys_attr1 = Units::getPhysicalAttrValue(d1->unit, sort_attr);
        int phys_attr2 = Units::getPhysicalAttrValue(d2->unit, sort_attr);
        if (descending){
            return phys_attr1 > phys_attr2;
        } else {
            return phys_attr2 > phys_attr1;
        }
    }
    if (sort_labor != unit_labor::NONE)
    {
        if (descending)
            return d1->unit->status.labors[sort_labor] > d2->unit->status.labors[sort_labor];
        else
            return d1->unit->status.labors[sort_labor] < d2->unit->status.labors[sort_labor];
    }
    return sortByName(d1, d2);
}

enum display_columns {
    DISP_COLUMN_HAPPINESS,
    DISP_COLUMN_NAME,
    DISP_COLUMN_PROFESSION_OR_SQUAD,
    DISP_COLUMN_DOING,
    DISP_COLUMN_SQUADS,
    DISP_COLUMN_LABORS,
    DISP_COLUMN_MAX,
};

class viewscreen_unitlaborsst : public dfhack_viewscreen {
public:
    void feed(set<df::interface_key> *events);

    void logic() {
        dfhack_viewscreen::logic();
        if (do_refresh_names)
            refreshNames();
    }

    void render();
    void resize(int w, int h) { calcSize(); }
    void sortUnits(altsort_mode);

    void help() { }

    std::string getFocusString() { return "unitlabors"; }

    df::unit *getSelectedUnit();

    viewscreen_unitlaborsst(vector<df::unit*> &src, int cursor_pos);
    ~viewscreen_unitlaborsst() { };

protected:
    vector<UnitInfo *> units;
    altsort_mode altsort;
    bool show_squad;

    bool do_refresh_names;
    int first_row, sel_row, num_rows;
    int first_column, sel_column;

    int col_widths[DISP_COLUMN_MAX];
    int col_offsets[DISP_COLUMN_MAX];

    void refreshNames();
    void calcSize ();

    std::vector<df::squad*> dwarf_squads;
    std::vector<df::squad*>::iterator it2;
};

viewscreen_unitlaborsst::viewscreen_unitlaborsst(vector<df::unit*> &src, int cursor_pos)
{
    std::map<df::unit*,int> active_idx;
    auto &active = world->units.active;
    for (size_t i = 0; i < active.size(); i++)
        active_idx[active[i]] = i;


    std::set<int> myset;
    std::set<int>::iterator it;
    it = myset.begin();

    for (size_t i = 0; i < src.size(); i++)
    {
        df::unit *unit = src[i];
        if (!unit)
        {
            if (cursor_pos > i)
                cursor_pos--;
            continue;
        }

        UnitInfo *cur = new UnitInfo;

        cur->unit = unit;
        cur->allowEdit = true;
        cur->active_index = active_idx[unit];

        if (unit->race != ui->race_id)
            cur->allowEdit = false;

        if (unit->civ_id != ui->civ_id)
            cur->allowEdit = false;

        if (unit->flags1.bits.dead)
            cur->allowEdit = false;

        if (!ENUM_ATTR(profession, can_assign_labor, unit->profession))
            cur->allowEdit = false;

        if(unit->civ_id == ui->civ_id){
            int msquad_id = unit->military.squad_id ;
            if(msquad_id > 0 && msquad_id < 100000){
                myset.insert(it, unit->military.squad_id);
                it++;
            }
        }

        cur->color = Units::getProfessionColor(unit);

        if(unit->profession != df::profession::BABY && unit->profession != df::profession::CHILD){
            units.push_back(cur);
        }
    }

    dwarf_squads = ui->squads.list;

    //for (it2=dwarf_squads.begin(); it2!=dwarf_squads.end(); ++it2){
    //    Core::print(stl_sprintf("hey got 1\n").c_str());
    //}
    //{
    //    auto squad = dwarf_squads[si];
    //    if(df.world. == squad->id){
    //    }
    //    //uint8_t c = 0xFA;
    //    //fg = 15;
    //    //if(cur->unit->military.squad_id == squad->id){
    //    //bg = 0;
    //    //    c = 0xFB;
    //    //}
    //}

    show_squad = true;
    first_column = sel_column = 0;

    refreshNames();
    sel_column = old_sel_column;

    if(old_altsort != ALTSORT_MAX){
        sortUnits(old_altsort);
        if(old_altsort == ALTSORT_SKILL){
            altsort = ALTSORT_NAME; 
        } else {
            altsort = old_altsort; 
        }
    }

    first_row = 0;
    sel_row = cursor_pos;
    calcSize();

    // recalculate first_row to roughly match the original layout
    first_row = 0;
    while (first_row < sel_row - num_rows + 1)
        first_row += num_rows + 2;
    // make sure the selection stays visible
    if (first_row > sel_row)
        first_row = sel_row - num_rows + 1;
    // don't scroll beyond the end
    if (first_row > units.size() - num_rows)
        first_row = units.size() - num_rows;
}

void viewscreen_unitlaborsst::refreshNames()
{
    do_refresh_names = false;

    for (size_t i = 0; i < units.size(); i++)
    {
        UnitInfo *cur = units[i];
        df::unit *unit = cur->unit;

        cur->name = Translation::TranslateName(Units::getVisibleName(unit), false);
        cur->transname = Translation::TranslateName(Units::getVisibleName(unit), true);
        cur->profession = Units::getProfessionName(unit);
        if (unit->military.squad_id > -1) {
            cur->squad_effective_name = Units::getSquadName(unit);
            cur->squad_info = stl_sprintf("%i", unit->military.squad_position + 1) + "." + cur->squad_effective_name;
        } else {
            cur->squad_effective_name = "";
            cur->squad_info = "";
        }
    }
    calcSize();
}

void viewscreen_unitlaborsst::calcSize()
{
    auto dim = Screen::getWindowSize();

    num_rows = dim.y - 11;
    if (num_rows > units.size())
        num_rows = units.size();

    int num_columns = dim.x - DISP_COLUMN_MAX - 1;

    // min/max width of columns
    int col_minwidth[DISP_COLUMN_MAX];
    int col_maxwidth[DISP_COLUMN_MAX];
    col_minwidth[DISP_COLUMN_HAPPINESS] = 4;
    col_maxwidth[DISP_COLUMN_HAPPINESS] = 4;
    col_minwidth[DISP_COLUMN_NAME] = 16;
    col_maxwidth[DISP_COLUMN_NAME] = 16;        // adjusted in the loop below
    col_minwidth[DISP_COLUMN_DOING] = 16;
    col_maxwidth[DISP_COLUMN_DOING] = 16;        // adjusted in the loop below
    col_minwidth[DISP_COLUMN_PROFESSION_OR_SQUAD] = 10;
    col_maxwidth[DISP_COLUMN_PROFESSION_OR_SQUAD] = 10;  // adjusted in the loop below
    col_minwidth[DISP_COLUMN_SQUADS] = 4;
    col_maxwidth[DISP_COLUMN_SQUADS] = 4;
    col_minwidth[DISP_COLUMN_LABORS] = num_columns*3/5;     // 60%
    col_maxwidth[DISP_COLUMN_LABORS] = NUM_COLUMNS;

    // get max_name/max_prof from strings length
    for (size_t i = 0; i < units.size(); i++)
    {
        if (col_maxwidth[DISP_COLUMN_NAME] < units[i]->name.size())
            col_maxwidth[DISP_COLUMN_NAME] = units[i]->name.size();
        if (show_squad) {
            if (col_maxwidth[DISP_COLUMN_PROFESSION_OR_SQUAD] < units[i]->squad_info.size())
                col_maxwidth[DISP_COLUMN_PROFESSION_OR_SQUAD] = units[i]->squad_info.size();
        } else {
            if (col_maxwidth[DISP_COLUMN_PROFESSION_OR_SQUAD] < units[i]->profession.size())
                col_maxwidth[DISP_COLUMN_PROFESSION_OR_SQUAD] = units[i]->profession.size();
        }
    }

    // check how much room we have
    int width_min = 0, width_max = 0;
    for (int i = 0; i < DISP_COLUMN_MAX; i++)
    {
        width_min += col_minwidth[i];
        width_max += col_maxwidth[i];
    }

    if (width_max <= num_columns)
    {
        // lots of space, distribute leftover (except last column)
        int col_margin   = (num_columns - width_max) / (DISP_COLUMN_MAX-1);
        int col_margin_r = (num_columns - width_max) % (DISP_COLUMN_MAX-1);
        for (int i = DISP_COLUMN_MAX-1; i>=0; i--)
        {
            col_widths[i] = col_maxwidth[i];

            if (i < DISP_COLUMN_MAX-1)
            {
                //col_widths[i] += col_margin;

                if (col_margin_r)
                {
                    col_margin_r--;
                    col_widths[i]++;
                }
            }
        }
    }
    else if (width_min <= num_columns)
    {
        // constrained, give between min and max to every column
        int space = num_columns - width_min;
        // max size columns not yet seen may consume
        int next_consume_max = width_max - width_min;

        for (int i = 0; i < DISP_COLUMN_MAX; i++)
        {
            // divide evenly remaining space
            int col_margin = space / (DISP_COLUMN_MAX-i);

            // take more if the columns after us cannot
            next_consume_max -= (col_maxwidth[i]-col_minwidth[i]);
            if (col_margin < space-next_consume_max)
                col_margin = space - next_consume_max;

            // no more than maxwidth
            if (col_margin > col_maxwidth[i] - col_minwidth[i])
                col_margin = col_maxwidth[i] - col_minwidth[i];

            col_widths[i] = col_minwidth[i] + col_margin;

            space -= col_margin;
        }
    }
    else
    {
        // should not happen, min screen is 80x25
        int space = num_columns;
        for (int i = 0; i < DISP_COLUMN_MAX; i++)
        {
            col_widths[i] = space / (DISP_COLUMN_MAX-i);
            space -= col_widths[i];
        }
    }

    for (int i = 0; i < DISP_COLUMN_MAX; i++)
    {
        if (i == 0)
            col_offsets[i] = 1;
        else
            col_offsets[i] = col_offsets[i - 1] + col_widths[i - 1] + 1;
    }

    // don't adjust scroll position immediately after the window opened
    if (units.size() == 0)
        return;

    // if the window grows vertically, scroll upward to eliminate blank rows from the bottom
    if (first_row > units.size() - num_rows)
        first_row = units.size() - num_rows;

    // if it shrinks vertically, scroll downward to keep the cursor visible
    if (first_row < sel_row - num_rows + 1)
        first_row = sel_row - num_rows + 1;

    // if the window grows horizontally, scroll to the left to eliminate blank columns from the right
    if (first_column > NUM_COLUMNS - col_widths[DISP_COLUMN_LABORS])
        first_column = NUM_COLUMNS - col_widths[DISP_COLUMN_LABORS];

    // if it shrinks horizontally, scroll to the right to keep the cursor visible
    if (first_column < sel_column - col_widths[DISP_COLUMN_LABORS] + 1)
        first_column = sel_column - col_widths[DISP_COLUMN_LABORS] + 1;
}

void viewscreen_unitlaborsst::feed(set<df::interface_key> *events)
{
    bool leave_all = events->count(interface_key::LEAVESCREEN_ALL);
    if (leave_all || events->count(interface_key::LEAVESCREEN))
    {
        old_sel_column = sel_column;
        events->clear();
        Screen::dismiss(this);
        if (leave_all)
        {
            events->insert(interface_key::LEAVESCREEN);
            parent->feed(events);
            events->clear();
        }
        return;
    }

    if (!units.size())
        return;

    if (do_refresh_names)
        refreshNames();

    int old_sel_row = sel_row;

    if (events->count(interface_key::CURSOR_UP) || events->count(interface_key::CURSOR_UPLEFT) || events->count(interface_key::CURSOR_UPRIGHT))
        sel_row--;
    if (events->count(interface_key::CURSOR_UP_FAST) || events->count(interface_key::CURSOR_UPLEFT_FAST) || events->count(interface_key::CURSOR_UPRIGHT_FAST))
        sel_row -= 10;
    if (events->count(interface_key::CURSOR_DOWN) || events->count(interface_key::CURSOR_DOWNLEFT) || events->count(interface_key::CURSOR_DOWNRIGHT))
        sel_row++;
    if (events->count(interface_key::CURSOR_DOWN_FAST) || events->count(interface_key::CURSOR_DOWNLEFT_FAST) || events->count(interface_key::CURSOR_DOWNRIGHT_FAST))
        sel_row += 10;

    if ((sel_row > 0) && events->count(interface_key::CURSOR_UP_Z_AUX))
    {
        sel_row = 0;
    }
    if ((sel_row < units.size()-1) && events->count(interface_key::CURSOR_DOWN_Z_AUX))
    {
        sel_row = units.size()-1;
    }

    if (sel_row < 0)
    {
        if (old_sel_row == 0 && events->count(interface_key::CURSOR_UP))
            sel_row = units.size() - 1;
        else
            sel_row = 0;
    }

    if (sel_row > units.size() - 1)
    {
        if (old_sel_row == units.size()-1 && events->count(interface_key::CURSOR_DOWN))
            sel_row = 0;
        else
            sel_row = units.size() - 1;
    }

    if (events->count(interface_key::STRING_A000))
        sel_row = 0;

    if (sel_row < first_row)
        first_row = sel_row;
    if (first_row < sel_row - num_rows + 1)
        first_row = sel_row - num_rows + 1;

    if (events->count(interface_key::CURSOR_LEFT) || events->count(interface_key::CURSOR_UPLEFT) || events->count(interface_key::CURSOR_DOWNLEFT))
        sel_column--;
    if (events->count(interface_key::CURSOR_LEFT_FAST) || events->count(interface_key::CURSOR_UPLEFT_FAST) || events->count(interface_key::CURSOR_DOWNLEFT_FAST))
        sel_column -= 10;
    if (events->count(interface_key::CURSOR_RIGHT) || events->count(interface_key::CURSOR_UPRIGHT) || events->count(interface_key::CURSOR_DOWNRIGHT))
        sel_column++;
    if (events->count(interface_key::CURSOR_RIGHT_FAST) || events->count(interface_key::CURSOR_UPRIGHT_FAST) || events->count(interface_key::CURSOR_DOWNRIGHT_FAST))
        sel_column += 10;

    if ((sel_column != 0) && events->count(interface_key::CURSOR_UP_Z))
    {
        // go to beginning of current column group; if already at the beginning, go to the beginning of the previous one
        sel_column--;
        int cur = columns[sel_column].group;
        while ((sel_column > 0) && columns[sel_column - 1].group == cur)
            sel_column--;
    }
    if ((sel_column != NUM_COLUMNS - 1) && events->count(interface_key::CURSOR_DOWN_Z))
    {
        // go to beginning of next group
        int cur = columns[sel_column].group;
        int next = sel_column+1;
        while ((next < NUM_COLUMNS) && (columns[next].group == cur))
            next++;
        if ((next < NUM_COLUMNS) && (columns[next].group != cur))
            sel_column = next;
    }

    if (events->count(interface_key::STRING_A000))
        sel_column = 0;

    if (sel_column < 0)
        sel_column = 0;
    if (sel_column > NUM_COLUMNS - 1)
        sel_column = NUM_COLUMNS - 1;

    if (events->count(interface_key::CURSOR_DOWN_Z) || events->count(interface_key::CURSOR_UP_Z))
    {
        // when moving by group, ensure the whole group is shown onscreen
        int endgroup_column = sel_column;
        while ((endgroup_column < NUM_COLUMNS-1) && columns[endgroup_column+1].group == columns[sel_column].group)
            endgroup_column++;

        if (first_column < endgroup_column - col_widths[DISP_COLUMN_LABORS] + 1)
            first_column = endgroup_column - col_widths[DISP_COLUMN_LABORS] + 1;
    }

    if (sel_column < first_column)
        first_column = sel_column;
    if (first_column < sel_column - col_widths[DISP_COLUMN_LABORS] + 1)
        first_column = sel_column - col_widths[DISP_COLUMN_LABORS] + 1;

    int input_row = sel_row;
    int input_column = sel_column;
    altsort_mode input_sort = altsort;

    // Translate mouse input to appropriate keyboard input
    if (enabler->tracking_on && gps->mouse_x != -1 && gps->mouse_y != -1)
    {
        int click_header = DISP_COLUMN_MAX; // group ID of the column header clicked
        int click_body = DISP_COLUMN_MAX; // group ID of the column body clicked

        int click_unit = -1; // Index into units[] (-1 if out of range)
        int click_labor = -1; // Index into columns[] (-1 if out of range)

        for (int i = 0; i < DISP_COLUMN_MAX; i++)
        {
            if ((gps->mouse_x >= col_offsets[i]) &&
                (gps->mouse_x < col_offsets[i] + col_widths[i]))
            {
                if ((gps->mouse_y >= 1) && (gps->mouse_y <= 2))
                    click_header = i;
                if ((gps->mouse_y >= 4) && (gps->mouse_y < 4 + num_rows))
                    click_body = i;
            }
        }

        if ((gps->mouse_x >= col_offsets[DISP_COLUMN_LABORS]) &&
            (gps->mouse_x < col_offsets[DISP_COLUMN_LABORS] + col_widths[DISP_COLUMN_LABORS]))
            click_labor = gps->mouse_x - col_offsets[DISP_COLUMN_LABORS] + first_column;
        if ((gps->mouse_y >= 4) && (gps->mouse_y < 4 + num_rows))
            click_unit = gps->mouse_y - 4 + first_row;

        switch (click_header)
        {
        case DISP_COLUMN_HAPPINESS:
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                input_sort = ALTSORT_HAPPINESS;
                if (enabler->mouse_lbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEUP);
                if (enabler->mouse_rbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEDOWN);
            }
            break;

        case DISP_COLUMN_NAME:
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                input_sort = ALTSORT_NAME;
                if (enabler->mouse_lbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEDOWN);
                if (enabler->mouse_rbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEUP);
            }
            break;

        case DISP_COLUMN_PROFESSION_OR_SQUAD:
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                input_sort = ALTSORT_PROFESSION_OR_SQUAD;
                if (enabler->mouse_lbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEDOWN);
                if (enabler->mouse_rbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEUP);
            }
            break;

        case DISP_COLUMN_LABORS:
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                input_column = click_labor;
                if (enabler->mouse_lbut)
                    events->insert(interface_key::SECONDSCROLL_UP);
                if (enabler->mouse_rbut)
                    events->insert(interface_key::SECONDSCROLL_DOWN);
            }
            break;
        }

        switch (click_body)
        {
        case DISP_COLUMN_HAPPINESS:
            // do nothing
            break;

        case DISP_COLUMN_NAME:
        case DISP_COLUMN_PROFESSION_OR_SQUAD:
            // left-click to view, right-click to zoom
            if (enabler->mouse_lbut)
            {
                input_row = click_unit;
                events->insert(interface_key::UNITJOB_VIEW);
            }
            if (enabler->mouse_rbut)
            {
                input_row = click_unit;
                events->insert(interface_key::UNITJOB_ZOOM_CRE);
            }
            break;

        case DISP_COLUMN_LABORS:
            // left-click to toggle, right-click to just highlight
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                if (enabler->mouse_lbut)
                {
                    input_row = click_unit;
                    input_column = click_labor;
                    events->insert(interface_key::SELECT);
                }
                if (enabler->mouse_rbut)
                {
                    sel_row = click_unit;
                    sel_column = click_labor;
                }
            }
            break;
        }
        enabler->mouse_lbut = enabler->mouse_rbut = 0;
    }

    UnitInfo *cur = units[input_row];
    if (events->count(interface_key::SELECT)) // && (cur->allowEdit) && columns[input_column].isValidLabor(ui->main.fortress_entity))
    {
        //df::unit *unit = cur->unit;
        //const SkillColumn &col = columns[input_column];
        //bool newstatus = !unit->status.labors[col.labor];
        //if (col.special)
        //{
        //    if (newstatus)
        //    {
        //        for (int i = 0; i < NUM_COLUMNS; i++)
        //        {
        //            if ((columns[i].labor != unit_labor::NONE) && columns[i].special)
        //                unit->status.labors[columns[i].labor] = false;
        //        }
        //    }
        //    unit->military.pickup_flags.bits.update = true;
        //}
        //unit->status.labors[col.labor] = newstatus;
        df::unit *unit = cur->unit;
        if(sel_column < dwarf_squads.size()){
            df::squad *squad = dwarf_squads.at(sel_column);
            if(unit->military.squad_id == -1){
                int next_open_position = -1;
                for (int i = 0; i < 10; i++){
                    if(squad->positions[i]->occupant == -1){
                        next_open_position = i;
                        break;
                    }
                }
                if(next_open_position != -1){
                    unit->military.pickup_flags.bits.update = true;
                    unit->military.squad_id = squad->id;
                    unit->military.squad_position = next_open_position;
                    squad->positions[next_open_position]->occupant = unit->hist_figure_id;
                }
            } else {
                for (int i = 0; i < 10; i++){
                    if(squad->positions[i]->occupant == unit->hist_figure_id){
                        unit->military.pickup_flags.bits.update = true;
                        squad->positions[i]->occupant = -1;
                        unit->military.squad_id = -1;
                    break;
                    }
                }
            }
        }
        refreshNames();
    }
    if (events->count(interface_key::SELECT_ALL))
    {
        //columns[input_column]
    }

    if (events->count(interface_key::SECONDSCROLL_UP) || events->count(interface_key::SECONDSCROLL_DOWN))
    {
        int actual_column = input_column - dwarf_squads.size();
        if(actual_column >= 0){
            descending = events->count(interface_key::SECONDSCROLL_UP);
            sort_skill = columns[actual_column].skill;
            sort_attr = columns[actual_column].attr;
            sort_labor = columns[actual_column].labor;
            sortUnits(ALTSORT_SKILL);
        }
    }

    if (events->count(interface_key::SECONDSCROLL_PAGEUP) || events->count(interface_key::SECONDSCROLL_PAGEDOWN))
    {
        descending = events->count(interface_key::SECONDSCROLL_PAGEUP);
    }
    if (events->count(interface_key::CHANGETAB))
    {
        sortUnits(input_sort);
    }
    if (events->count(interface_key::OPTION20))
    {
        show_squad = !show_squad;
    }

    if (VIRTUAL_CAST_VAR(unitlist, df::viewscreen_unitlistst, parent))
    {
        if (events->count(interface_key::UNITJOB_VIEW) || events->count(interface_key::UNITJOB_ZOOM_CRE))
        {
            for (int i = 0; i < unitlist->units[unitlist->page].size(); i++)
            {
                if (unitlist->units[unitlist->page][i] == units[input_row]->unit)
                {
                    unitlist->cursor_pos[unitlist->page] = i;
                    unitlist->feed(events);
                    if (Screen::isDismissed(unitlist))
                        Screen::dismiss(this);
                    else
                        do_refresh_names = true;
                    break;
                }
            }
        }
    }
}

void viewscreen_unitlaborsst::sortUnits(altsort_mode input_sort){
    switch (input_sort)
    {
    case ALTSORT_NAME:
        std::sort(units.begin(), units.end(), sortByName);
        break;
    case ALTSORT_PROFESSION_OR_SQUAD:
        std::sort(units.begin(), units.end(), show_squad ? sortBySquad : sortByProfession);
        break;
    case ALTSORT_HAPPINESS:
        std::sort(units.begin(), units.end(), sortByHappiness);
        break;
    case ALTSORT_ARRIVAL:
        std::sort(units.begin(), units.end(), sortByArrival);
        break;
    case ALTSORT_SKILL:
        std::sort(units.begin(), units.end(), sortBySkill);
        break;
    }
    old_altsort = input_sort;
}

void OutputString(int8_t color, int &x, int y, const std::string &text)
{
    Screen::paintString(Screen::Pen(' ', color, 0), x, y, text);
    x += text.length();
}
void viewscreen_unitlaborsst::render()
{
    if (Screen::isDismissed(this))
        return;

    dfhack_viewscreen::render();

    auto dim = Screen::getWindowSize();

    Screen::clear();
    Screen::drawBorder("  Dwarf Manipulator - Manage Labors  ");

    Screen::paintString(Screen::Pen(' ', 7, 0), col_offsets[DISP_COLUMN_HAPPINESS], 2, "Hap.");
    Screen::paintString(Screen::Pen(' ', 7, 0), col_offsets[DISP_COLUMN_NAME], 2, "Name");
    Screen::paintString(Screen::Pen(' ', 7, 0), col_offsets[DISP_COLUMN_PROFESSION_OR_SQUAD], 2, show_squad ? "Squad" : "Profession");
    Screen::paintString(Screen::Pen(' ', 7, 0), col_offsets[DISP_COLUMN_DOING], 2, "Doing");
    int squad_offset = 0;
    int squad_count = dwarf_squads.size();
    for (it2 = dwarf_squads.begin(); it2 != dwarf_squads.end(); ++it2){
        int col_offset = squad_offset + first_column;
        int8_t fg = 1;
        int8_t bg = 0;
        if (col_offset == sel_column)
        {
            fg = 0;
            bg = 7;
        }
        Screen::paintTile(Screen::Pen('S', fg, bg), col_offsets[DISP_COLUMN_SQUADS] + squad_offset, 1);
        Screen::paintTile(Screen::Pen('S', fg, bg), col_offsets[DISP_COLUMN_SQUADS] + squad_offset, 2);
        //Core::print(stl_sprintf("hey got 1 %d\n", col_offsets[DISP_COLUMN_LABORS] + squad_offset).c_str());
        squad_offset++;
    }

    for (int col = 0; col < col_widths[DISP_COLUMN_LABORS]; col++)
    {
        int col_offset = col + first_column;
        if (col_offset >= NUM_COLUMNS)
            break;

        int8_t fg = columns[col_offset].color;
        int8_t bg = 0;

        if (col_offset + squad_count == sel_column)
        {
            fg = 0;
            bg = 7;
        }

        Screen::paintTile(Screen::Pen(columns[col_offset].label[0], fg, bg), col_offsets[DISP_COLUMN_LABORS] + col, 1);
        Screen::paintTile(Screen::Pen(columns[col_offset].label[1], fg, bg), col_offsets[DISP_COLUMN_LABORS] + col, 2);
        //Core::print(stl_sprintf("hey got 1 %d\n", col_offsets[DISP_COLUMN_LABORS] + col + 4).c_str());
        df::profession profession = columns[col_offset].profession;
        if ((profession != profession::NONE) && (ui->race_id != -1))
        {
            auto graphics = world->raws.creatures.all[ui->race_id]->graphics;
            Screen::paintTile(
                Screen::Pen(' ', fg, 0,
                    graphics.profession_add_color[creature_graphics_role::DEFAULT][profession],
                    graphics.profession_texpos[creature_graphics_role::DEFAULT][profession]),
                col_offsets[DISP_COLUMN_LABORS] + col, 3);
        }
    }

    for (int row = 0; row < num_rows; row++)
    {
        int row_offset = row + first_row;
        if (row_offset >= units.size())
            break;

        UnitInfo *cur = units[row_offset];
        df::unit *unit = cur->unit;
        int8_t fg = 15, bg = 0;

        int happy = cur->unit->status.happiness;
        string happiness = stl_sprintf("%4i", happy);
        if (happy == 0)         // miserable
            fg = 13;    // 5:1
        else if (happy <= 25)   // very unhappy
            fg = 12;    // 4:1
        else if (happy <= 50)   // unhappy
            fg = 4;     // 4:0
        else if (happy < 75)    // fine
            fg = 14;    // 6:1
        else if (happy < 125)   // quite content
            fg = 6;     // 6:0
        else if (happy < 150)   // happy
            fg = 2;     // 2:0
        else                    // ecstatic
            fg = 10;    // 2:1
        Screen::paintString(Screen::Pen(' ', fg, bg), col_offsets[DISP_COLUMN_HAPPINESS], 4 + row, happiness);

        fg = 15;
        if (row_offset == sel_row)
        {
            fg = 0;
            bg = 7;
        }

        string name = cur->name;
        name.resize(col_widths[DISP_COLUMN_NAME]);
        Screen::paintString(Screen::Pen(' ', fg, bg), col_offsets[DISP_COLUMN_NAME], 4 + row, name);

        bg = 0;
        string profession_or_squad;
        if (show_squad) {
            fg = 11;
            profession_or_squad = cur->squad_info;
        } else {
            fg = cur->color;
            profession_or_squad = cur->profession;
        }
        profession_or_squad.resize(col_widths[DISP_COLUMN_PROFESSION_OR_SQUAD]);
        Screen::paintString(Screen::Pen(' ', fg, bg), col_offsets[DISP_COLUMN_PROFESSION_OR_SQUAD], 4 + row, profession_or_squad);

        string doing = "";
        if(cur->unit->job.current_job != 0){
                doing = ENUM_KEY_STR(job_type, cur->unit->job.current_job->job_type);
        }
        doing.resize(col_widths[DISP_COLUMN_DOING]);
        Screen::paintString(Screen::Pen(' ', fg, bg), col_offsets[DISP_COLUMN_DOING], 4 + row, doing);
        int squad_index = 0;
        for (it2 = dwarf_squads.begin(); it2 != dwarf_squads.end(); ++it2){
            fg = 15;
            bg = 0;
            if (row_offset == sel_row)
            {
                fg = 0;
                bg = 15;
            }
            uint8_t c = 0xFA;
            if(cur->unit->military.squad_id == (*it2)->id){
                c = 0xFB;
            }
            Screen::paintTile(Screen::Pen(c, fg, bg), col_offsets[DISP_COLUMN_SQUADS] + squad_index, 4 + row);
            squad_index++;
        }

        // Print unit's skills and labor assignments
        for (int col = 0; col < col_widths[DISP_COLUMN_LABORS]; col++)
        {
            int col_offset = col + first_column;
            fg = 15;
            bg = 0;
            uint8_t c = 0xFA;
            if ((col_offset + squad_count == sel_column) && (row_offset == sel_row))
                fg = 9;
            if (columns[col_offset].skill != job_skill::NONE)
            {
                df::unit_skill *skill = NULL;
                if (unit->status.current_soul)
                    skill = binsearch_in_vector<df::unit_skill,df::job_skill>(unit->status.current_soul->skills, &df::unit_skill::id, columns[col_offset].skill);
                if ((skill != NULL) && (skill->rating || skill->experience))
                {
                    int level = skill->rating;
                    if (level > NUM_SKILL_LEVELS - 1)
                        level = NUM_SKILL_LEVELS - 1;
                    c = skill_levels[level].abbrev;
                }
                else
                    c = '-';
            } else {
                // TODO: use max attribute value to properly make every fit into hex
                string phys_attr = stl_sprintf("%x", Units::getPhysicalAttrValue(cur->unit, columns[col_offset].attr) / 200);
                c = phys_attr.c_str()[0];
            }
            if (columns[col_offset].labor != unit_labor::NONE)
            {
                if (unit->status.labors[columns[col_offset].labor])
                {
                    bg = 7;
                    if (columns[col_offset].skill == job_skill::NONE)
                        c = 0xF9;
                }
            }
            else
                bg = 3;
            Screen::paintTile(Screen::Pen(c, fg, bg), col_offsets[DISP_COLUMN_LABORS] + col, 4 + row);
        }
    }

    UnitInfo *cur = units[sel_row];
    bool canToggle = false;
    if (cur != NULL)
    {
        df::unit *unit = cur->unit;
        int x = 1, y = 3 + num_rows + 2;
        Screen::Pen white_pen(' ', 15, 0);

        Screen::paintString(white_pen, x, y, (cur->unit && cur->unit->sex) ? "\x0b" : "\x0c");
        x += 2;
        Screen::paintString(white_pen, x, y, cur->transname);
        x += cur->transname.length();

        if (cur->transname.length())
        {
            Screen::paintString(white_pen, x, y, ", ");
            x += 2;
        }
        Screen::paintString(white_pen, x, y, cur->profession);
        x += cur->profession.length();
        Screen::paintString(white_pen, x, y, ": ");
        x += 2;

        string str;
        int actual_column = sel_column - squad_count;
        if(actual_column < 0){
            str = stl_sprintf("%s", dwarf_squads[sel_column]->alias.c_str());
            //return Translation::TranslateName(&squad->name, true);
        }
        else if (columns[actual_column].skill == job_skill::NONE)
        {
            str = ENUM_ATTR_STR(unit_labor, caption, columns[actual_column].labor);
            if (unit->status.labors[columns[actual_column].labor])
                str += " Enabled";
            else
                str = stl_sprintf("%s %d", columns[actual_column].label, Units::getPhysicalAttrValue(unit, columns[actual_column].attr));
                //str += " Not Enabled";
        }
        else
        {
            df::unit_skill *skill = NULL;
            if (unit->status.current_soul)
                skill = binsearch_in_vector<df::unit_skill,df::job_skill>(unit->status.current_soul->skills, &df::unit_skill::id, columns[actual_column].skill);
            if (skill)
            {
                int level = skill->rating;
                if (level > NUM_SKILL_LEVELS - 1)
                    level = NUM_SKILL_LEVELS - 1;
                str = stl_sprintf("%s %s", skill_levels[level].name, ENUM_ATTR_STR(job_skill, caption_noun, columns[actual_column].skill));
                if (level != NUM_SKILL_LEVELS - 1)
                    str += stl_sprintf(" (%d/%d)", skill->experience, skill_levels[level].points);
            }
            else
                str = stl_sprintf("Not %s (0/500)", ENUM_ATTR_STR(job_skill, caption_noun, columns[actual_column].skill));
        }
        Screen::paintString(Screen::Pen(' ', 9, 0), x, y, str);

        if (cur->unit->military.squad_id > -1) {

            x = 1;
            y++;

            string squadLabel = "Squad: ";
            Screen::paintString(white_pen, x, y, squadLabel);
            x += squadLabel.size();

            string squad = cur->squad_effective_name;
            Screen::paintString(Screen::Pen(' ', 11, 0), x, y, squad);
            x += squad.size();

            string pos = stl_sprintf(" Pos %i", cur->unit->military.squad_position + 1);
            Screen::paintString(Screen::Pen(' ', 9, 0), x, y, pos);

        }

        canToggle = (cur->allowEdit) && columns[sel_column].isValidLabor(ui->main.fortress_entity);
    }

    int x = 2, y = dim.y - 3;
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::SELECT));
    OutputString(canToggle ? 15 : 8, x, y, ": Toggle labor, ");

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::SELECT_ALL));
    OutputString(canToggle ? 15 : 8, x, y, ": Toggle Group, ");

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::UNITJOB_VIEW));
    OutputString(15, x, y, ": ViewCre, ");

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::UNITJOB_ZOOM_CRE));
    OutputString(15, x, y, ": Zoom-Cre");

    x = 2; y = dim.y - 2;
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::LEAVESCREEN));
    OutputString(15, x, y, ": Done, ");

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::OPTION20));
    OutputString(15, x, y, ": Toggle View, ");
    
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::SECONDSCROLL_DOWN));
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::SECONDSCROLL_UP));
    OutputString(15, x, y, ": Sort by Skill, ");

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::SECONDSCROLL_PAGEDOWN));
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::SECONDSCROLL_PAGEUP));
    OutputString(15, x, y, ": Sort by (");
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CHANGETAB));
    OutputString(15, x, y, ") ");
    switch (altsort)
    {
    case ALTSORT_NAME:
        OutputString(15, x, y, "Name");
        break;
    case ALTSORT_PROFESSION_OR_SQUAD:
        OutputString(15, x, y, show_squad ? "Squad" : "Profession");
        break;
    case ALTSORT_HAPPINESS:
        OutputString(15, x, y, "Happiness");
        break;
    case ALTSORT_ARRIVAL:
        OutputString(15, x, y, "Arrival");
        break;
    default:
        OutputString(15, x, y, "Unknown");
        break;
    }
}

df::unit *viewscreen_unitlaborsst::getSelectedUnit()
{
    // This query might be from the rename plugin
    do_refresh_names = true;

    return units[sel_row]->unit;
}

struct unitlist_hook : df::viewscreen_unitlistst
{
    typedef df::viewscreen_unitlistst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (input->count(interface_key::CUSTOM_N))
        {
            if (units[page].size())
            {
                Screen::show(new viewscreen_unitlaborsst(units[page], cursor_pos[page]));
                return;
            }
        }
        INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();

        if (units[page].size())
        {
            auto dim = Screen::getWindowSize();
            int x = 42, y = dim.y - 2;
            OutputString(12, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_N));
            OutputString(15, x, y, ": Manage squads (DFHack)");
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(unitlist_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(unitlist_hook, render);

DFHACK_PLUGIN("commander");

DFHACK_PLUGIN_IS_ENABLED(is_enabled);

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (!gps)
        return CR_FAILURE;

    if (enable != is_enabled)
    {
        if (!INTERPOSE_HOOK(unitlist_hook, feed).apply(enable) ||
            !INTERPOSE_HOOK(unitlist_hook, render).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    INTERPOSE_HOOK(unitlist_hook, feed).remove();
    INTERPOSE_HOOK(unitlist_hook, render).remove();
    return CR_OK;
}
