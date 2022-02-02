// SPDX-FileCopyrightText: 2022 RizinOrg <info@rizin.re>
// SPDX-FileCopyrightText: 2022 deroad <wargio@libero.it>
// SPDX-License-Identifier: LGPL-3.0-only
#include "yara_common.h"

/** \file yara_plugin.c
 * Adds core plugin to rizin to handle yara rules
 */

#undef RZ_API
#define RZ_API static
#undef RZ_IPI
#define RZ_IPI static

#define SETDESC(x, y)     rz_config_node_desc(x, y)
#define SETPREFS(x, y, z) SETDESC(rz_config_set(cfg, x, y), z)
#define SETPREFI(x, y, z) SETDESC(rz_config_set_i(cfg, x, y), z)
#define SETPREFB(x, y, z) SETDESC(rz_config_set_b(cfg, x, y), z)

static HtPP *yara_metadata = NULL;

static const RzCmdDescHelp yara_command_grp_help = {
	.summary = "Rizin custom yara parser and generator of YARA rules.",
};

static const RzCmdDescHelp yara_command_flag_grp_help = {
	.summary = "Lists or adds or removes yara strings used to generate rules.",
};

static const RzCmdDescHelp yara_command_flag_add_grp_help = {
	.summary = "Add yara strings used to generate rules.",
};

static const RzCmdDescHelp yara_command_metadata_grp_help = {
	.summary = "Adds/Removes/Lists metadata used when generating rules.",
};

static const RzCmdDescArg yara_command_main_args[] = {
	{ 0 },
};

static const RzCmdDescHelp yara_command_main_help = {
	.summary = "yara commands and examples.",
	.args = yara_command_main_args,
};

static const RzCmdDescArg yara_command_create_args[] = {
	{
		.name = "rulename",
		.type = RZ_CMD_ARG_TYPE_STRING,
	},
	{ 0 },
};

static const RzCmdDescHelp yara_command_create_help = {
	.summary = "Creates a new rule at the current offset",
	.args = yara_command_create_args,
};

static const RzCmdDescArg yara_command_load_args[] = {
	{
		.name = "file",
		.type = RZ_CMD_ARG_TYPE_FILE,
	},
	{ 0 },
};

static const RzCmdDescHelp yara_command_load_help = {
	.summary = "Parse a .yar/.yara file and applies the rules",
	.args = yara_command_load_args,
};

static const RzCmdDescArg yara_command_folder_args[] = {
	{
		.name = "folder",
		.type = RZ_CMD_ARG_TYPE_FILE,
	},
	{ 0 },
};

static const RzCmdDescHelp yara_command_folder_help = {
	.summary = "Searches for .yar/.yara files in a folder recursively and applies the rules",
	.args = yara_command_folder_args,
};

static const RzCmdDescArg yara_command_metadata_list_args[] = {
	{ 0 },
};

static const RzCmdDescHelp yara_command_metadata_list_help = {
	.summary = "Lists metadata used when generating rules.",
	.args = yara_command_metadata_list_args,
};

static const RzCmdDescArg yara_command_metadata_add_args[] = {
	{
		.name = "name",
		.type = RZ_CMD_ARG_TYPE_STRING,
	},
	{
		.name = "value",
		.type = RZ_CMD_ARG_TYPE_STRING,
		.optional = true,
	},
	{ 0 },
};

static const RzCmdDescHelp yara_command_metadata_add_help = {
	.summary = "Adds metadata used when generating rules.",
	.args = yara_command_metadata_add_args,
};

static const RzCmdDescArg yara_command_metadata_remove_args[] = {
	{
		.name = "name",
		.type = RZ_CMD_ARG_TYPE_STRING,
	},
	{ 0 },
};

static const RzCmdDescHelp yara_command_metadata_remove_help = {
	.summary = "Removes metadata used when generating rules.",
	.args = yara_command_metadata_remove_args,
};

static const RzCmdDescArg yara_command_flag_list_args[] = {
	{ 0 },
};

static const RzCmdDescHelp yara_command_flag_list_help = {
	.summary = "Lists yara strings used to generate rules.",
	.args = yara_command_flag_list_args,
};

static const RzCmdDescArg yara_command_flag_add_auto_args[] = {
	{
		.name = "name",
		.type = RZ_CMD_ARG_TYPE_STRING,
	},
	{
		.name = "#bytes",
		.type = RZ_CMD_ARG_TYPE_NUM,
		.optional = true,
	},
	{ 0 },
};

static const RzCmdDescHelp yara_command_flag_add_auto_help = {
	.summary = "Adds a yara strings used when generating rules (auto type detection).",
	.args = yara_command_flag_add_auto_args,
};

static const RzCmdDescArg yara_command_flag_add_string_args[] = {
	{
		.name = "name",
		.type = RZ_CMD_ARG_TYPE_STRING,
	},
	{
		.name = "#bytes",
		.type = RZ_CMD_ARG_TYPE_NUM,
		.optional = true,
	},
	{ 0 },
};

static const RzCmdDescHelp yara_command_flag_add_string_help = {
	.summary = "Adds a yara strings used when generating rules (string type).",
	.args = yara_command_flag_add_string_args,
};

static const RzCmdDescArg yara_command_flag_add_bytes_args[] = {
	{
		.name = "name",
		.type = RZ_CMD_ARG_TYPE_STRING,
	},
	{
		.name = "#bytes",
		.type = RZ_CMD_ARG_TYPE_NUM,
	},
	{ 0 },
};

static const RzCmdDescHelp yara_command_flag_add_bytes_help = {
	.summary = "Adds a yara strings used when generating rules (bytes type).",
	.args = yara_command_flag_add_bytes_args,
};

static const RzCmdDescArg yara_command_flag_add_masked_asm_args[] = {
	{
		.name = "name",
		.type = RZ_CMD_ARG_TYPE_STRING,
	},
	{
		.name = "#bytes",
		.type = RZ_CMD_ARG_TYPE_NUM,
	},
	{ 0 },
};

static const RzCmdDescHelp yara_command_flag_add_masked_asm_help = {
	.summary = "Adds a yara strings used when generating rules (masked assembly type).",
	.args = yara_command_flag_add_masked_asm_args,
};

static const RzCmdDescArg yara_command_flag_add_unmasked_asm_args[] = {
	{
		.name = "name",
		.type = RZ_CMD_ARG_TYPE_STRING,
	},
	{
		.name = "#bytes",
		.type = RZ_CMD_ARG_TYPE_NUM,
	},
	{ 0 },
};

static const RzCmdDescHelp yara_command_flag_add_unmasked_asm_help = {
	.summary = "Adds a yara strings used when generating rules (unmasked assembly type).",
	.args = yara_command_flag_add_unmasked_asm_args,
};

static const RzCmdDescArg yara_command_flag_remove_args[] = {
	{
		.name = "flag name",
		.type = RZ_CMD_ARG_TYPE_FLAG,
	},
	{ 0 },
};

static const RzCmdDescHelp yara_command_flag_remove_help = {
	.summary = "Removes a yara strings used when generating rules.",
	.args = yara_command_flag_remove_args,
};

static const RzCmdDescArg yara_command_flag_clean_args[] = {
	{ 0 },
};

static const RzCmdDescHelp yara_command_flag_clean_help = {
	.summary = "Removes all the yara strings used when generating rules.",
	.args = yara_command_flag_clean_args,
};

static void yara_command_load_error(bool is_warning, const char *file, int line, const RzYaraRule *rule, const char *message, void *user_data) {
	if (is_warning) {
		YARA_WARN("%s:%d: %s\n", file, line, message);
		return;
	}
	YARA_ERROR("%s:%d: %s\n", file, line, message);
}

static void yara_add_match_flag(RzCore *core, const RzYaraMatch *match) {
	char flagname[256];
	const char *mem_loc = "va";
	ut64 offset = rz_io_p2v(core->io, match->offset);
	if (offset == UT64_MAX) {
		mem_loc = "pa";
		offset = match->offset;
	}
	rz_strf(flagname, RZ_YARA_FLAG_PREFIX_MATCH "%s.%s_%s_%" PFMT64x, mem_loc, match->rule, match->string + 1, match->offset);
	rz_flag_space_push(core->flags, RZ_YARA_FLAG_SPACE_MATCH);
	rz_flag_set(core->flags, flagname, offset, match->size);
	rz_flag_space_pop(core->flags);
}

RZ_IPI RzCmdStatus yara_command_load_handler(RzCore *core, int argc, const char **argv) {
	RzYaraMatch *ym = NULL;
	int timeout_secs = 0;
	RzList *matches = NULL;
	RzListIter *it = NULL;
	RzYaraRules *rules = NULL;
	RzYaraScanner *scanner = NULL;
	RzYaraCompiler *comp = NULL;
	bool fast_mode = false;

	comp = rz_yara_compiler_new(yara_command_load_error, NULL);
	if (!comp || !rz_yara_compiler_parse_file(comp, argv[1])) {
		rz_warn_if_reached();
		rz_yara_compiler_free(comp);
		return RZ_CMD_STATUS_ERROR;
	}

	fast_mode = rz_config_get_b(core->config, RZ_YARA_CFG_FASTMODE);
	timeout_secs = rz_config_get_i(core->config, RZ_YARA_CFG_TIMEOUT);
	if (timeout_secs < 1) {
		YARA_WARN(RZ_YARA_CFG_TIMEOUT " is set to an invalid number. using 5min timeout.\n");
		// timeout 5 Mins
		timeout_secs = 5 * 60;
	}
	rules = rz_yara_compiler_get_rules_and_free(comp);
	scanner = rz_yara_scanner_new(rules, timeout_secs, fast_mode);
	if (!scanner) {
		rz_warn_if_reached();
		rz_yara_rules_free(rules);
		return RZ_CMD_STATUS_ERROR;
	}

	matches = rz_yara_scanner_search(scanner, core);
	rz_yara_scanner_free(scanner);
	rz_yara_rules_free(rules);

	rz_list_foreach (matches, it, ym) {
		yara_add_match_flag(core, ym);
	}
	rz_cons_printf("%u matches (check f~" RZ_YARA_FLAG_PREFIX_MATCH ")\n", rz_list_length(matches));
	rz_list_free(matches);

	return matches ? RZ_CMD_STATUS_OK : RZ_CMD_STATUS_ERROR;
}

RZ_IPI RzCmdStatus yara_command_folder_handler(RzCore *core, int argc, const char **argv) {
	RzYaraMatch *ym = NULL;
	const char *element = NULL;
	const char *ext = NULL;
	int dir_depth = 0;
	ut32 loaded = 0;
	int timeout_secs = 0;
	RzList *list = NULL;
	RzList *extensions = NULL;
	RzListIter *it = NULL;
	RzListIter *it2 = NULL;
	RzYaraRules *rules = NULL;
	RzYaraScanner *scanner = NULL;
	RzYaraCompiler *comp = NULL;
	bool fast_mode = false;
	char path[1024];

	if (!rz_file_is_directory(argv[1])) {
		YARA_ERROR("'%s' is not a directory.\n", argv[1]);
		return RZ_CMD_STATUS_WRONG_ARGS;
	}

	dir_depth = rz_config_get_i(core->config, "dir.depth");
	ext = rz_config_get(core->config, RZ_YARA_CFG_EXTENSIONS);
	if (RZ_STR_ISEMPTY(ext)) {
		ext = YARA_DEFAULT_EXT;
	}
	fast_mode = rz_config_get_b(core->config, RZ_YARA_CFG_FASTMODE);
	timeout_secs = rz_config_get_i(core->config, RZ_YARA_CFG_TIMEOUT);
	if (timeout_secs < 1) {
		YARA_WARN(RZ_YARA_CFG_TIMEOUT " is set to an invalid number. using 5min timeout.\n");
		// timeout 5 Mins
		timeout_secs = 5 * 60;
	}

	extensions = rz_str_split_duplist(ext, ",", true);
	if (!extensions) {
		YARA_ERROR("cannnot allocate extensions list.\n");
		return RZ_CMD_STATUS_ERROR;
	}

	rz_strf(path, "%s" RZ_SYS_DIR "**", argv[1]);
	list = rz_file_globsearch(path, dir_depth);
	if (rz_list_length(list) < 1) {
		ext = rz_config_get(core->config, RZ_YARA_CFG_EXTENSIONS);
		YARA_ERROR("'%s' directory does not contain any %s files.\n", argv[1], ext);
		rz_list_free(list);
		rz_list_free(extensions);
		return RZ_CMD_STATUS_ERROR;
	}

	if (!(comp = rz_yara_compiler_new(yara_command_load_error, NULL))) {
		rz_list_free(list);
		rz_list_free(extensions);
		return RZ_CMD_STATUS_ERROR;
	}

	rz_list_foreach (list, it, element) {
		rz_list_foreach (extensions, it2, ext) {
			if (!rz_str_endswith(element, ext)) {
				continue;
			} else if (!rz_yara_compiler_parse_file(comp, element)) {
				rz_yara_compiler_free(comp);
				rz_list_free(list);
				rz_list_free(extensions);
				return RZ_CMD_STATUS_ERROR;
			}
			YARA_INFO("loaded file %s\n", element);
			loaded++;
			break;
		}
	}
	rz_list_free(list);
	rz_list_free(extensions);

	if (loaded < 1) {
		ext = rz_config_get(core->config, RZ_YARA_CFG_EXTENSIONS);
		YARA_ERROR("'%s' directory does not contain any %s files.\n", argv[1], ext);
		rz_yara_compiler_free(comp);
		return RZ_CMD_STATUS_ERROR;
	}

	rules = rz_yara_compiler_get_rules_and_free(comp);
	scanner = rz_yara_scanner_new(rules, timeout_secs, fast_mode);
	if (!scanner) {
		rz_warn_if_reached();
		rz_yara_rules_free(rules);
		return RZ_CMD_STATUS_ERROR;
	}

	list = rz_yara_scanner_search(scanner, core);
	rz_yara_scanner_free(scanner);
	rz_yara_rules_free(rules);

	rz_list_foreach (list, it, ym) {
		yara_add_match_flag(core, ym);
	}
	rz_cons_printf("%u matches (check f~" RZ_YARA_FLAG_PREFIX_MATCH ")\n", rz_list_length(list));
	rz_list_free(list);

	return list ? RZ_CMD_STATUS_OK : RZ_CMD_STATUS_ERROR;
}

RZ_IPI RzCmdStatus yara_command_create_handler(RzCore *core, int argc, const char **argv) {
	char *rule = rz_yara_create_rule_from_bytes(core, yara_metadata, argv[1]);
	if (!rule) {
		return RZ_CMD_STATUS_ERROR;
	}

	rz_cons_printf("%s", rule);
	free(rule);
	return RZ_CMD_STATUS_OK;
}

static bool yara_flag_list_standard(RzFlagItem *fi, void *unused) {
	(void)unused;
	rz_cons_printf("0x%" PFMT64x " 0x%" PFMT64x " %s\n", fi->offset, fi->size, fi->name);
	return true;
}

static bool yara_flag_list_quiet(RzFlagItem *fi, void *unused) {
	(void)unused;
	rz_cons_printf("%s\n", fi->name);
	return true;
}

static bool yara_flag_list_json(RzFlagItem *fi, PJ *pj) {
	pj_o(pj);
	pj_ks(pj, "name", fi->name);
	pj_kn(pj, "offset", fi->offset);
	pj_kn(pj, "size", fi->size);
	pj_end(pj);
	return true;
}

static bool yara_flag_list_table(RzFlagItem *fi, RzTable *table) {
	rz_table_add_rowf(table, "Xxs", fi->offset, fi->size, fi->name);
	return true;
}

RZ_IPI RzCmdStatus yara_command_flag_list_handler(RzCore *core, int argc, const char **argv, RzCmdStateOutput *state) {
	RzCmdStatus res = RZ_CMD_STATUS_OK;
	switch (state->mode) {
	case RZ_OUTPUT_MODE_STANDARD:
		rz_flag_foreach_glob(core->flags, RZ_YARA_FLAG_SPACE_RULE, (RzFlagItemCb)yara_flag_list_standard, NULL);
		break;
	case RZ_OUTPUT_MODE_QUIET:
		rz_flag_foreach_glob(core->flags, RZ_YARA_FLAG_SPACE_RULE, (RzFlagItemCb)yara_flag_list_quiet, NULL);
		break;
	case RZ_OUTPUT_MODE_JSON:
		pj_a(state->d.pj);
		rz_flag_foreach_glob(core->flags, RZ_YARA_FLAG_SPACE_RULE, (RzFlagItemCb)yara_flag_list_json, state->d.pj);
		pj_end(state->d.pj);
		break;
	case RZ_OUTPUT_MODE_TABLE:
		rz_table_set_columnsf(state->d.t, "Xxs", "offset", "size", "name", NULL);
		rz_flag_foreach_glob(core->flags, RZ_YARA_FLAG_SPACE_RULE, (RzFlagItemCb)yara_flag_list_table, state->d.t);
		break;
	default:
		rz_warn_if_reached();
		res = RZ_CMD_STATUS_WRONG_ARGS;
		break;
	}
	return res;
}

RZ_IPI RzCmdStatus yara_command_flag_remove_handler(RzCore *core, int argc, const char **argv) {
	if (strncmp(argv[1], RZ_YARA_FLAG_SPACE_RULE, strlen(RZ_YARA_FLAG_SPACE_RULE))) {
		YARA_ERROR("%s is not a yara rule flag (" RZ_YARA_FLAG_SPACE_RULE ".*)\n", argv[1]);
		return RZ_CMD_STATUS_WRONG_ARGS;
	}
	rz_flag_unset_name(core->flags, argv[1]);
	return RZ_CMD_STATUS_OK;
}

RZ_IPI RzCmdStatus yara_command_flag_clean_handler(RzCore *core, int argc, const char **argv) {
	rz_flag_unset_all_in_space(core->flags, RZ_YARA_FLAG_SPACE_RULE);
	return RZ_CMD_STATUS_OK;
}

static bool yara_is_valid_name(const char *name) {
	int len = strlen(name);
	if (len < 1) {
		YARA_ERROR("string name is empty.\n");
		return false;
	} else if (len > 128) {
		YARA_ERROR("string name is too lon (max 128 chars).\n");
		return false;
	}
	for (int i = 0; i < len; ++i) {
		if (i > 0 && IS_DIGIT(name[i])) {
			continue;
		} else if (i > 0 && name[i] == '_') {
			continue;
		} else if (IS_UPPER(name[i]) || IS_LOWER(name[i])) {
			continue;
		}
		YARA_ERROR("string name contains an invalid char at %d (%c).\n", i, name[i]);
		YARA_ERROR("accepted values are only A-Z, a-z, _ and the name must start with a letter\n");
		return false;
	}
	return true;
}

RZ_IPI RzCmdStatus yara_command_flag_add_auto_handler(RzCore *core, int argc, const char **argv) {
	char flagname[256];
	bool is_string = false;
	bool is_asm = false;
	const char *name = argv[1];
	ut64 n_bytes = 0;
	if (argc == 3) {
		n_bytes = rz_get_input_num_value(NULL, argv[2]);
	}
	if (!yara_is_valid_name(name)) {
		return RZ_CMD_STATUS_WRONG_ARGS;
	}

	RzFlagItem *found = rz_flag_get_at(core->flags, core->offset, false);
	if (found) {
		RzList *tmp = NULL;
		if (rz_str_startswith(found->name, RZ_YARA_FLAG_SPACE_RULE)) {
			YARA_ERROR("there is already a yara string defined at 0x%" PFMT64x "\n", core->offset);
			return RZ_CMD_STATUS_ERROR;
		} else if (rz_str_startswith(found->name, "str.")) {
			n_bytes = n_bytes == 0 ? found->size : n_bytes;
			is_string = true;
		} else if ((tmp = rz_analysis_get_functions_in(core->analysis, core->offset)) && rz_list_length(tmp) > 0) {
			is_asm = true;
		}
		rz_list_free(tmp);
	}

	if (n_bytes < 1 || n_bytes > 0x1000) {
		YARA_ERROR("invalid number of bytes (expected n between 1 and 0x1000)\n");
		return RZ_CMD_STATUS_WRONG_ARGS;
	}

	if (is_string) {
		rz_strf(flagname, RZ_YARA_FLAG_PREFIX_STRING "%s", name);
	} else if (is_asm) {
		rz_strf(flagname, RZ_YARA_FLAG_PREFIX_ASM_M "%s", name);
	} else {
		rz_strf(flagname, RZ_YARA_FLAG_PREFIX_BYTES "%s", name);
	}

	if (rz_flag_get(core->flags, flagname)) {
		YARA_ERROR("yara string, named '%s', already exists\n", flagname);
		return RZ_CMD_STATUS_WRONG_ARGS;
	}

	rz_flag_space_push(core->flags, RZ_YARA_FLAG_SPACE_RULE);
	rz_flag_set(core->flags, flagname, core->offset, n_bytes);
	rz_flag_space_pop(core->flags);
	return RZ_CMD_STATUS_OK;
}

RZ_IPI RzCmdStatus yara_command_flag_add_string_handler(RzCore *core, int argc, const char **argv) {
	char flagname[256];
	const char *name = argv[1];
	ut64 n_bytes = 0;
	if (argc == 3) {
		n_bytes = rz_get_input_num_value(NULL, argv[2]);
	} else {
		RzFlagItem *found = rz_flag_get_at(core->flags, core->offset, false);
		if (!found) {
			YARA_ERROR("cannot find string at 0x%" PFMT64x "\n", core->offset);
			return RZ_CMD_STATUS_ERROR;
		} else if (rz_str_startswith(found->name, RZ_YARA_FLAG_SPACE_RULE)) {
			YARA_ERROR("there is already a yara string defined at 0x%" PFMT64x "\n", core->offset);
			return RZ_CMD_STATUS_ERROR;
		} else if (rz_str_startswith(found->name, "str.")) {
			n_bytes = found->size;
		}
	}
	if (n_bytes < 1 || n_bytes > 0x1000) {
		YARA_ERROR("invalid number of bytes (expected n between 1 and 0x1000)\n");
		return RZ_CMD_STATUS_WRONG_ARGS;
	}

	rz_strf(flagname, RZ_YARA_FLAG_PREFIX_STRING "%s", name);
	if (rz_flag_get(core->flags, flagname)) {
		YARA_ERROR("yara string, named '%s', already exists\n", flagname);
		return RZ_CMD_STATUS_WRONG_ARGS;
	}

	rz_flag_space_push(core->flags, RZ_YARA_FLAG_SPACE_RULE);
	rz_flag_set(core->flags, flagname, core->offset, n_bytes);
	rz_flag_space_pop(core->flags);
	return RZ_CMD_STATUS_OK;
}

RZ_IPI RzCmdStatus yara_command_flag_add_bytes_handler(RzCore *core, int argc, const char **argv) {
	char flagname[256];
	const char *name = argv[1];
	ut64 n_bytes = rz_get_input_num_value(NULL, argv[2]);

	if (n_bytes < 1 || n_bytes > 0x1000) {
		YARA_ERROR("invalid number of bytes (expected n between 1 and 0x1000)\n");
		return RZ_CMD_STATUS_WRONG_ARGS;
	}

	rz_strf(flagname, RZ_YARA_FLAG_PREFIX_BYTES "%s", name);
	if (rz_flag_get(core->flags, flagname)) {
		YARA_ERROR("yara string, named '%s', already exists\n", flagname);
		return RZ_CMD_STATUS_WRONG_ARGS;
	}

	rz_flag_space_push(core->flags, RZ_YARA_FLAG_SPACE_RULE);
	rz_flag_set(core->flags, flagname, core->offset, n_bytes);
	rz_flag_space_pop(core->flags);
	return RZ_CMD_STATUS_OK;
}

RZ_IPI RzCmdStatus yara_command_flag_add_masked_asm_handler(RzCore *core, int argc, const char **argv) {
	char flagname[256];
	const char *name = argv[1];
	ut64 n_bytes = rz_get_input_num_value(NULL, argv[2]);

	if (n_bytes < 1 || n_bytes > 0x1000) {
		YARA_ERROR("invalid number of bytes (expected n between 1 and 0x1000)\n");
		return RZ_CMD_STATUS_WRONG_ARGS;
	}

	rz_strf(flagname, RZ_YARA_FLAG_PREFIX_ASM_M "%s", name);
	if (rz_flag_get(core->flags, flagname)) {
		YARA_ERROR("yara string, named '%s', already exists\n", flagname);
		return RZ_CMD_STATUS_WRONG_ARGS;
	}

	rz_flag_space_push(core->flags, RZ_YARA_FLAG_SPACE_RULE);
	rz_flag_set(core->flags, flagname, core->offset, n_bytes);
	rz_flag_space_pop(core->flags);
	return RZ_CMD_STATUS_OK;
}

RZ_IPI RzCmdStatus yara_command_flag_add_unmasked_asm_handler(RzCore *core, int argc, const char **argv) {
	char flagname[256];
	const char *name = argv[1];
	ut64 n_bytes = rz_get_input_num_value(NULL, argv[2]);

	if (n_bytes < 1 || n_bytes > 0x1000) {
		YARA_ERROR("invalid number of bytes (expected n between 1 and 0x1000)\n");
		return RZ_CMD_STATUS_WRONG_ARGS;
	}

	rz_strf(flagname, RZ_YARA_FLAG_PREFIX_ASM_U "%s", name);
	if (rz_flag_get(core->flags, flagname)) {
		YARA_ERROR("yara string, named '%s', already exists\n", flagname);
		return RZ_CMD_STATUS_WRONG_ARGS;
	}

	rz_flag_space_push(core->flags, RZ_YARA_FLAG_SPACE_RULE);
	rz_flag_set(core->flags, flagname, core->offset, n_bytes);
	rz_flag_space_pop(core->flags);
	return RZ_CMD_STATUS_OK;
}

static bool yara_metadata_list_standard(void *unused, const char *k, const char *v) {
	(void)unused;
	if (RZ_STR_ISEMPTY(v) && (is_keyword_hash(k) || is_keyword_date(k))) {
		rz_cons_printf("%s = <auto filled>\n", k);
	} else {
		rz_cons_printf("%s = %s\n", k, v);
	}
	return true;
}

static bool yara_metadata_list_quiet(void *unused, const char *k, const char *v) {
	(void)unused;
	rz_cons_printf("%s %s\n", k, v);
	return true;
}

static bool yara_metadata_list_json(PJ *pj, const char *k, const char *v) {
	if (!strcmp(v, YARA_KEYWORD_TRUE) || !strcmp(v, YARA_KEYWORD_FALSE)) {
		pj_kb(pj, k, v[0] == 't');
	} else if (rz_is_valid_input_num_value(NULL, v)) {
		ut64 num = rz_get_input_num_value(NULL, v);
		pj_kn(pj, k, num);
	} else {
		pj_ks(pj, k, v);
	}
	return true;
}

static bool yara_metadata_list_table(RzTable *table, const char *k, const char *v) {
	if (RZ_STR_ISEMPTY(v) && (is_keyword_hash(k) || is_keyword_date(k))) {
		rz_table_add_rowf(table, "ss", k, "<auto filled>");
	} else {
		rz_table_add_rowf(table, "ss", k, v);
	}
	return true;
}

RZ_IPI RzCmdStatus yara_command_metadata_list_handler(RzCore *core, int argc, const char **argv, RzCmdStateOutput *state) {
	RzCmdStatus res = RZ_CMD_STATUS_OK;
	switch (state->mode) {
	case RZ_OUTPUT_MODE_STANDARD:
		ht_pp_foreach(yara_metadata, (HtPPForeachCallback)yara_metadata_list_standard, NULL);
		break;
	case RZ_OUTPUT_MODE_QUIET:
		ht_pp_foreach(yara_metadata, (HtPPForeachCallback)yara_metadata_list_quiet, NULL);
		break;
	case RZ_OUTPUT_MODE_JSON:
		pj_o(state->d.pj);
		ht_pp_foreach(yara_metadata, (HtPPForeachCallback)yara_metadata_list_json, state->d.pj);
		pj_end(state->d.pj);
		break;
	case RZ_OUTPUT_MODE_TABLE:
		rz_table_set_columnsf(state->d.t, "ss", "key", "value", NULL);
		ht_pp_foreach(yara_metadata, (HtPPForeachCallback)yara_metadata_list_table, state->d.t);
		break;
	default:
		rz_warn_if_reached();
		res = RZ_CMD_STATUS_WRONG_ARGS;
		break;
	}
	return res;
}

RZ_IPI RzCmdStatus yara_command_metadata_add_handler(RzCore *core, int argc, const char **argv) {
	const char *value = NULL;
	if (argc != 3) {
		if (!is_keyword_hash(argv[1]) && !is_keyword_date(argv[1])) {
			YARA_ERROR("missing value for key '%s'\n", argv[1]);
			return RZ_CMD_STATUS_WRONG_ARGS;
		}
		value = "";
	} else if (is_keyword_boolean(argv[2])) {
		// ensure that value is valid for yaml
		value = (!yara_stricmp(argv[2], YARA_KEYWORD_TRUE) ? YARA_KEYWORD_TRUE : YARA_KEYWORD_FALSE);
	} else {
		value = argv[2];
	}
	ht_pp_update(yara_metadata, argv[1], (void *)value);
	return RZ_CMD_STATUS_OK;
}

RZ_IPI RzCmdStatus yara_command_metadata_remove_handler(RzCore *core, int argc, const char **argv) {
	ht_pp_delete(yara_metadata, argv[1]);
	return RZ_CMD_STATUS_OK;
}

/*
RZ_IPI RzCmdStatus yara_command_main_handler(RzCore *core, int argc, const char **argv) {
	const char *usage = ""
			    "commands:\n"
			    "  yaras add <name> <#bytes> # to create a new yara string\n"
			    "  yaras del <name>          # to remove a yara string\n"
			    "  yaras list                # to list all yara strings\n"
			    "  yaras clean               # to remove all yara string\n"
			    "  yarac <rulename>          # to create a new rule\n"
			    "  yarad <directory>         # to loads all yara files and applies to the binary\n"
			    "  yaral <file>              # to load a yara file and apply to the binary\n"
			    "  yaram add <key> <value>   # adds a metadata key value (used by yarac)\n"
			    "  yaram del <key>           # removes a metadata key\n"
			    "  yaram list <key>          # lists all metadata keys\n"
			    "\n"
			    "usage examples:\n"
			    "  to create a rule\n"
			    "    yarasa rooted_00 @ str.rooted\n"
			    "    yarasa shell_code 10 @ fcn.0x123+2\n"
			    "    yarasa aes_sbox 256 @ 0x1234\n"
			    "    yarasa mistake 256 @ 0xdeadbeef\n"
			    "    yarasd yara.bytes.mistake\n"
			    "    yarac bad_malware\n"
			    "\n"
			    "  to add metadata when creating a rule\n"
			    "    yaram add author \"john foo\"\n"
			    "    yaram add thread_level 3\n"
			    "    yaram add is_elf true\n"
			    "    yaram add date \"\" # if empty, generates one (see also 'el " RZ_YARA_CFG_DATE_FMT "')\n"
			    "  to remove a metadata key\n"
			    "    yaram del is_elf\n"
			    "  to list all the metadata key/values\n"
			    "    yaram list";

	rz_cons_println(usage);
	return RZ_CMD_STATUS_OK;
}
*/

RZ_IPI bool yara_plugin_init(RzCore *core) {
	yara_metadata = rz_yara_metadata_new();
	if (!yara_metadata) {
		YARA_ERROR("cannot allocate metadata hashmap\n");
		return false;
	}

	RzCmd *rcmd = core->rcmd;
	RzConfig *cfg = core->config;
	RzCmdDesc *root_cd = rz_cmd_get_root(rcmd);
	if (!root_cd) {
		rz_warn_if_reached();
		return false;
	}

	rz_config_lock(cfg, false);
	SETPREFS(RZ_YARA_CFG_TAGS, "", "yara rule tags to use in the rule tag location when generating rules (space separated).");
	SETPREFS(RZ_YARA_CFG_EXTENSIONS, YARA_DEFAULT_EXT, "yara file extensions, comma separated (default " YARA_DEFAULT_EXT ").");
	SETPREFS(RZ_YARA_CFG_DATE_FMT, "%Y-%m-%d", "yara metadata date format (uses strftime for formatting).");
	SETPREFI(RZ_YARA_CFG_TIMEOUT, 5 * 60, "yara scanner timeout in seconds (default: 5mins).");
	SETPREFB(RZ_YARA_CFG_FASTMODE, false, "yara scanner fast mode, skips multiple matches (default: false).");
	rz_config_lock(cfg, true);

	RzCmdDesc *yara_cd = rz_cmd_desc_group_new(rcmd, root_cd, "yara", NULL, &yara_command_main_help, &yara_command_grp_help);
	rz_return_val_if_fail(yara_cd, false);

	RzCmdDesc *yara_create_cd = rz_cmd_desc_argv_new(rcmd, yara_cd, "yarac", yara_command_create_handler, &yara_command_create_help);
	rz_return_val_if_fail(yara_create_cd, false);

	RzCmdDesc *yara_folder_cd = rz_cmd_desc_argv_new(rcmd, yara_cd, "yarad", yara_command_folder_handler, &yara_command_folder_help);
	rz_return_val_if_fail(yara_folder_cd, false);

	RzCmdDesc *yara_load_cd = rz_cmd_desc_argv_new(rcmd, yara_cd, "yaral", yara_command_load_handler, &yara_command_load_help);
	rz_return_val_if_fail(yara_load_cd, false);

	// yara metadata group

	int yaram_modes = RZ_OUTPUT_MODE_STANDARD | RZ_OUTPUT_MODE_QUIET | RZ_OUTPUT_MODE_JSON | RZ_OUTPUT_MODE_TABLE;
	RzCmdDesc *yara_meta_cd = rz_cmd_desc_group_state_new(rcmd, yara_cd, "yaram", yaram_modes, yara_command_metadata_list_handler, &yara_command_metadata_list_help, &yara_command_metadata_grp_help);
	rz_return_val_if_fail(yara_meta_cd, false);

	RzCmdDesc *yara_meta_add_cd = rz_cmd_desc_argv_new(rcmd, yara_meta_cd, "yarama", yara_command_metadata_add_handler, &yara_command_metadata_add_help);
	rz_return_val_if_fail(yara_meta_add_cd, false);

	RzCmdDesc *yara_meta_remove_cd = rz_cmd_desc_argv_new(rcmd, yara_meta_cd, "yaramr", yara_command_metadata_remove_handler, &yara_command_metadata_remove_help);
	rz_return_val_if_fail(yara_meta_remove_cd, false);

	// yara flags group

	int yaras_modes = RZ_OUTPUT_MODE_STANDARD | RZ_OUTPUT_MODE_QUIET | RZ_OUTPUT_MODE_JSON | RZ_OUTPUT_MODE_TABLE;
	RzCmdDesc *yara_flag_cd = rz_cmd_desc_group_state_new(rcmd, yara_cd, "yaras", yaras_modes, yara_command_flag_list_handler, &yara_command_flag_list_help, &yara_command_flag_grp_help);
	rz_return_val_if_fail(yara_flag_cd, false);

	RzCmdDesc *yara_flag_add_cd = rz_cmd_desc_group_new(rcmd, yara_flag_cd, "yarasa", yara_command_flag_add_auto_handler, &yara_command_flag_add_auto_help, &yara_command_flag_add_grp_help);
	rz_return_val_if_fail(yara_flag_add_cd, false);

	RzCmdDesc *yara_flag_clean_cd = rz_cmd_desc_argv_new(rcmd, yara_flag_cd, "yarasc", yara_command_flag_clean_handler, &yara_command_flag_clean_help);
	rz_return_val_if_fail(yara_flag_clean_cd, false);

	RzCmdDesc *yara_flag_remove_cd = rz_cmd_desc_argv_new(rcmd, yara_flag_cd, "yarasr", yara_command_flag_remove_handler, &yara_command_flag_remove_help);
	rz_return_val_if_fail(yara_flag_remove_cd, false);

	// yara flags add options

	RzCmdDesc *yara_flag_add_bytes_cd = rz_cmd_desc_argv_new(rcmd, yara_flag_add_cd, "yarasab", yara_command_flag_add_bytes_handler, &yara_command_flag_add_bytes_help);
	rz_return_val_if_fail(yara_flag_add_bytes_cd, false);

	RzCmdDesc *yara_flag_add_string_cd = rz_cmd_desc_argv_new(rcmd, yara_flag_add_cd, "yarasas", yara_command_flag_add_string_handler, &yara_command_flag_add_string_help);
	rz_return_val_if_fail(yara_flag_add_string_cd, false);

	RzCmdDesc *yara_flag_add_masm_cd = rz_cmd_desc_argv_new(rcmd, yara_flag_add_cd, "yarasam", yara_command_flag_add_masked_asm_handler, &yara_command_flag_add_masked_asm_help);
	rz_return_val_if_fail(yara_flag_add_masm_cd, false);

	RzCmdDesc *yara_flag_add_uasm_cd = rz_cmd_desc_argv_new(rcmd, yara_flag_add_cd, "yarasau", yara_command_flag_add_unmasked_asm_handler, &yara_command_flag_add_unmasked_asm_help);
	rz_return_val_if_fail(yara_flag_add_uasm_cd, false);

	if (yr_initialize() != ERROR_SUCCESS) {
		rz_warn_if_reached();
		return false;
	}

	return true;
}

RZ_IPI bool yara_plugin_fini(RzCore *core) {
	yr_finalize();
	ht_pp_free(yara_metadata);
	return true;
}

RzCorePlugin rz_core_plugin_yara = {
	.name = "rz_yara",
	.author = "deroad",
	.desc = "Rizin YARA rules parser and generator.",
	.license = "LGPL-3.0",
	.init = yara_plugin_init,
	.fini = yara_plugin_fini,
};

#ifdef _MSC_VER
#define RZ_EXPORT __declspec(dllexport)
#else
#define RZ_EXPORT
#endif

#ifndef CORELIB
RZ_EXPORT RzLibStruct rizin_plugin = {
	.type = RZ_LIB_TYPE_CORE,
	.data = &rz_core_plugin_yara,
	.version = RZ_VERSION,
	.pkgname = "rz_yara"
};
#endif
