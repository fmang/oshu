/**
 * \file beatmap/parser.h
 * \ingroup beatmap
 *
 * \brief
 * Internal header for the parser.
 *
 * Welcome to oshu's beatmap parser.
 *
 * Writing a parser is no trivial task, especially for a chaotic format like
 * osu's. However, with some organisation, it could be done well and
 * consistently.
 *
 * Let's first explain why we're not using lex. Standard token parsers don't
 * have a lot of contextual information, and while lex is powerful enough to
 * virtually allow anything, it's probably going to get complicated. Imagine
 * you have a *float* token, which parses things that looks like numbers. Also
 * imagine some song is named 123. It's no easy task telling lex that because
 * of token appears in the right section, and after the right key, it should
 * not interpret it as a number but as a string. The osu beatmap format is full
 * of cases like that, so let's have some fun and do something cooler that
 * abusing tools.
 *
 * Moving on to the implementation details.
 *
 * The #parser_state is the structure passed to every function of this module.
 * It contains the beatmap object that we're creating, a string buffer
 * containing the input, and any transient state required for parsing.
 *
 * A *parser* is a function that reads input from the parser state, and returns
 * the parsed object. See #parse_int, #parse_float.
 *
 * A *consumer* is a special parser that returns nothing. See #consume_string,
 * #consume_space, #consume_end.
 *
 * A *processor* is a kind of parser that don't return the parsed state to the
 * caller, but instead writes its results directly into the beatmap referenced
 * in the parser state.
 *
 * All these functions return 0 on success, and -1 on failure. The callee
 * should print an message using #parser_error or #parser_warning on failure.
 */

#include "beatmap/beatmap.h"

/**
 * Enumerate all the special strings that may be found in a header file.
 *
 * Using such a structure makes it easier, and also faster, to branch on
 * sections or keys. The #parse_token function also optimizes the performance
 * by performing a dichotomic search.
 *
 * Since we're in an internal header, let's break the naming a bit and use the
 * same strings as the ones in the beatmap, with the same case.
 */
enum token {
#define TOKEN(t) t,
#include "beatmap/tokens.h"
#undef TOKEN
NUM_TOKENS,
};

/**
 * Enumeration to keep track of the section we're currently in.
 *
 * This is especially important before the format inside each section changes
 * significantly, some being INI-like, and others being CSV-like.
 *
 * The sections are listed in the order they should appear in in the beatmap
 * file.
 *
 * The value of the constants are the same as their tokens, so that conversion
 * from token to section is trivial.
 */
enum beatmap_section {
	BEATMAP_HEADER = -3, /**< Expect the #osu_file_header. */
	BEATMAP_ROOT = -2, /**< Between the header and the first section. We expect nothing. */
	BEATMAP_UNKNOWN = -1,
	BEATMAP_GENERAL = General, /**< INI-like. */
	BEATMAP_EDITOR = Editor, /**< INI-like. */
	BEATMAP_METADATA = Metadata, /**< INI-like. */
	BEATMAP_DIFFICULTY = Difficulty, /**< INI-like. */
	BEATMAP_EVENTS = Events, /**< CSV-like. */
	BEATMAP_TIMING_POINTS = TimingPoints, /**< CSV-like. */
	BEATMAP_COLOURS = Colours, /**< INI-like. */
	BEATMAP_HIT_OBJECTS = HitObjects, /**< CSV-like. */
};

/**
 * The parsing process is split into many functions, each minding its own
 * business.
 *
 * To make the prototypes of these functions easy, all they pass in the line to
 * parse and that parser state structure.
 *
 * You'll note the parsing is based on an automaton model, with a transition
 * function receiving one line at a time, and updating the parser state.
 */
struct parser_state {
	/**
	 * The current input buffer.
	 *
	 * Since the parser operates line-by-line, the input buffer starts with
	 * a full line. As the parsing process advances, the cursor is
	 * incremented, until it reaches the end terminator. It may be NULL
	 * when there is nothing left to parse.
	 *
	 * \todo
	 * Once we stop using strsep, this field should never be null, so this
	 * opens room for simplification.
	 */
	char *input;
	/**
	 * The initial input state when reading a line.
	 *
	 * Useful to determine the column number, using the difference between
	 * #input and #buffer.
	 */
	char *buffer;
	/**
	 * The line number corresponding to the current #input, starting at 1.
	 *
	 * If 0, it means the parsing process hasn't begun.
	 */
	int line_number;
	/**
	 * The name of the file being read, or something similar, like <stdin>
	 * or some URL, who knows.
	 */
	const char *source;
	/**
	 * The beatmap object we're filling.
	 *
	 * Its memory isn't handled by us.
	 */
	struct oshu_beatmap *beatmap;
	/**
	 * The current section.
	 *
	 * At the beginning, it's #BEATMAP_HEADER, meaning the header hasn't
	 * been parsed yet. Then it becomes #BEATMAP_ROOT, which should not
	 * contain anything.
	 *
	 * Then, as sections appear like `[General]`, it takes the value of the
	 * matching token, as mentionned in #beatmap_section. If the section is
	 * unknown, #section becomes #BEATMAP_UNKNOWN.
	 */
	enum beatmap_section section;
	/**
	 * Keep track of the last timing point to build the linked list.
	 */
	struct oshu_timing_point *last_timing_point;
	/**
	 * When parsing hit objects, we need to figure out what its timing
	 * point is, notably to compute slider durations.
	 *
	 * It is NULL until the first hit object is parsed, and keeps increasing.
	 * It must not become NULL at the end of the chain, because otherwise
	 * we're back to the first case and will trigger an infinite loop.
	 */
	struct oshu_timing_point *current_timing_point;
	/**
	 * This is the last non-inherited timing point.
	 */
	struct oshu_timing_point *timing_base;
	/**
	 * Keep track of the last hit object to build the linked list.
	 */
	struct oshu_hit *last_hit;
};

/**
 * Log an error with contextual information from the parser state.
 *
 * \sa oshu_log_error
 */
static void parser_error(struct parser_state *parser, const char *message);

/**
 * Convient typedef to help define the prototypes. Don't use it in the
 * implementation.
 */
typedef struct parser_state P;

/*
 * Here are the primitives, they may be called anywhere as they do generic
 * tasks.
 */

static int consume_all(P*);
static int consume_end(P*);
static int consume_spaces(P*);
static int consume_string(P*, const char*);

static int parse_int(P*, int*);
static int parse_double(P*, double*);
static int parse_string(P*, char**);
static int parse_token(P*, enum token*);
static int parse_key(P*, enum token*);

/*
 * From this point you'll have a pseudo call-graph of the complex parsers and
 * processors.
 */

static int process_input(P*);
	static int process_header(P*);
	static int process_section(P*);
	static int process_general(P*);
		static int parse_sample_set(P*, enum oshu_sample_set_family*);
	static int process_metadata(P*);
	static int process_difficulty(P*);
/*
	static int process_event(P*);
	static int process_timing_point(P*);
		static int parse_timing_point(P*, struct oshu_timing_point*);
	static int process_colour(P*);
	static int process_hit_object(P*);
		static int parse_hit_object(P*, struct oshu_hit*);
			static int parse_slider(P*, struct oshu_hit*);
				static int parse_point(P*, struct oshu_point*);
			static int parse_spinner(P*, struct oshu_hit*);
			static int parse_hold_note(P*, struct oshu_hit*);
*/
