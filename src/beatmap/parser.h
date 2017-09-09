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
 * should print an message using #parser_error or #parser_warn on failure.
 */

#include "beatmap/beatmap.h"

/**
 * Enumeration to keep track of the section we're currently in.
 *
 * This is especially important before the format inside each section changes
 * significantly, some being INI-like, and others being CSV-like.
 */
enum beatmap_section {
	BEATMAP_HEADER = 0, /**< Expect the #osu_file_header. */
	BEATMAP_ROOT, /**< Between the header and the first section. We expect nothing. */
	BEATMAP_GENERAL, /**< INI-like. */
	BEATMAP_EDITOR, /**< INI-like. */
	BEATMAP_METADATA, /**< INI-like. */
	BEATMAP_DIFFICULTY, /**< INI-like. */
	BEATMAP_EVENTS, /**< CSV-like. */
	BEATMAP_TIMING_POINTS, /**< CSV-like. */
	BEATMAP_COLOURS, /**< INI-like. */
	BEATMAP_HIT_OBJECTS, /**< CSV-like. */
	BEATMAP_UNKNOWN,
};

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
	BEGIN_CATEGORIES,
	HEADER, /**< Expect the #osu_file_header. */
	ROOT, /**< Between the header and the first section. We expect nothing. */
	General, /**< INI-like. */
	Editor, /**< INI-like. */
	Metadata, /**< INI-like. */
	Difficulty, /**< INI-like. */
	Events, /**< CSV-like. */
	TimingPoints, /**< CSV-like. */
	Colours, /**< INI-like. */
	HitObjects, /**< CSV-like. */
	UNKNOWN,
	END_CATEGORIES,

	BEGIN_GENERAL,
	AudioFileName,
	AudioLeadIn,
	PreviewTime,
	Countdown,
	SampleSet,
	StackLeniency,
	Mode,
	LetterboxInBreaks,
	WidescreenStoryBoard,
	END_GENERAL,
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
	 */
	char *input;
	int line_number;
	struct oshu_beatmap *beatmap;
	enum beatmap_section section;
	struct oshu_hit *last_hit;
	struct oshu_timing_point *last_timing_point;
	struct oshu_timing_point *current_timing_point;
	/** This is the last non-inherited timing point. */
	struct oshu_timing_point *timing_base;
};

/**
 * Log an error with contextual information from the parser state.
 *
 * \sa oshu_log_error
 */
static void parser_error(struct parser_state *parser, const char *fmt, ...);

/**
 * Log a warning with contextual information from the parser state.
 *
 * \sa oshu_log_warn
 */
static void parser_warn(struct parser_state *parser, const char *fmt, ...);

/**
 * Convient typedef to help define the prototypes. Don't use it in the
 * implementation.
 */
typedef struct parser_state P;

/*
 * Here are the primitives, they may be called anywhere as they do generic
 * tasks.
 */

/*
static int consume_string(P*, const char*);
static int consume_space(P*);
static int consume_end(P*);

static int parse_int(P*, int*);
static int parse_double(P*, double*);
static int parse_token(P*, enum token*);
static int parse_key(P*, enum token*);
*/

/*
 * From this point you'll have a pseudo call-graph of the complex parsers and
 * processors.
 */

/*
static int process_input(P*);
	static int process_category(P*);
	static int process_general(P*);
	static int process_metadata(P*);
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
