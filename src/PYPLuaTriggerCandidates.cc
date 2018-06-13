/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2018 Peng Wu <alexepico@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "PYPLuaTriggerCandidates.h"
#include <assert.h>
#include "PYString.h"
#include "PYConfig.h"
#include "PYPPhoneticEditor.h"

using namespace PY;

LuaTriggerCandidates::LuaTriggerCandidates (PhoneticEditor *editor)
{
    m_editor = editor;

    m_lua_plugin = ibus_engine_plugin_new ();

    loadLuaScript ( ".." G_DIR_SEPARATOR_S "lua" G_DIR_SEPARATOR_S "base.lua")||
        loadLuaScript (PKGDATADIR G_DIR_SEPARATOR_S "base.lua");

    gchar * path = g_build_filename (g_get_user_config_dir (),
                             "ibus", "libpinyin", "user.lua", NULL);
    loadLuaScript(path);
    g_free(path);
}

int
LuaTriggerCandidates::loadLuaScript (std::string filename)
{
    return !ibus_engine_plugin_load_lua_script
        (m_lua_plugin, filename.c_str ());
}

gboolean
LuaTriggerCandidates::processCandidates (std::vector<EnhancedCandidate> & candidates)
{
    EnhancedCandidate enhanced;
    enhanced.m_candidate_type = CANDIDATE_LUA_TRIGGER;
    enhanced.m_candidate_id = 0;

    std::vector<EnhancedCandidate>::iterator pos;
    for (pos = candidates.begin (); pos != candidates.end (); ++pos) {
        if (CANDIDATE_NBEST_MATCH != pos->m_candidate_type)
            break;
    }

    const char * lua_function_name = NULL;
    const char * text = m_editor->m_text;

    if (ibus_engine_plugin_match_input
        (m_lua_plugin, text, &lua_function_name)) {
        ibus_engine_plugin_call(m_lua_plugin, lua_function_name, text);
        enhanced.m_display_string =
            ibus_engine_plugin_get_first_result (m_lua_plugin);

        candidates.insert (pos, enhanced);
        return TRUE;
    } else {
        int num = std::min
            (m_editor->m_config.pageSize (), (guint)candidates.size ());
        for (int i = 0; i < num; ++i) {
            text = candidates[i].m_display_string.c_str ();
            if (ibus_engine_plugin_match_candidate
                (m_lua_plugin, text, &lua_function_name)) {
                ibus_engine_plugin_call(m_lua_plugin, lua_function_name, text);
                enhanced.m_display_string =
                    ibus_engine_plugin_get_first_result (m_lua_plugin);

                candidates.insert (pos, enhanced);
                return TRUE;
            }
        }
    }

    return FALSE;
}

SelectCandidateAction
LuaTriggerCandidates::selectCandidate (EnhancedCandidate & enhanced)
{
    assert (CANDIDATE_LUA_TRIGGER == enhanced.m_candidate_type);
    assert (0 == enhanced.m_candidate_id);

    return SELECT_CANDIDATE_COMMIT;
}