Vibecoding Lexicon 200

**Background & benchmark concept**  
**Timestamp:** 2026-10-17 18:29 MDT  
**Status:** Publication draft

## Summary

Generative & agentic AI may act as a de-facto accelerator of language
normalization. A model repeatedly selects one word & one meaning from several
plausible alternatives. Its output then enters software, reports, policies,
schools, search results, & later AI-mediated writing.

This can improve consistency. It can also make dominant meanings still more
dominant, hide legitimate alternatives, & turn an ordinary ambiguity into an
AI hallucination (AIH), AI task error (AITE), or unsafe software action.

The practical question for vibecoding is:

> Which words should be avoided or explicitly defined in a particular context,
> & what are the likely consequences if a model selects the wrong meaning?

That question can support both a development guide & a model benchmark.

## Words operate in context

An English word is not a self-contained instruction. Its operative meaning
depends on linguistic, social, historical, & task context. The comparison with
C is useful: a token or expression is interpreted through syntax, scope, type,
surrounding state, & implementation rules.

Natural language is less constrained. A word can carry everyday, technical,
community-specific, metaphorical, ironic, & disputed meanings. Context does
not always eliminate ambiguity; sometimes it preserves or creates it.

The umbrella term is **lexical ambiguity**. **Polysemy** normally describes
related senses of one word. **Homonymy** describes meanings treated as distinct
despite sharing the same form. **Word-sense disambiguation** is the task of
selecting the intended meaning from context.

Examples:

- **Order:** a sequence, command, purchase, biological rank, or condition of
  procedural regularity.
- **Agent:** a representative, causal substance, software component, or
  autonomous AI system.
- **Hallucination:** a human perceptual experience or an AI output described as
  invented, unsupported, inconsistent, or otherwise erroneous.
- **Security:** safety, protective controls, a financial instrument, collateral,
  or a professional function.
- **Fallacy:** an invalid argument form, mistaken belief, or misleading
  argumentative move.
- **Fluent:** "native speech", smooth language production, language competence, 
  or effective participation in a specialized discourse.

The last example exposes an important distinction. **Surface fluency** is smooth,
grammatical, plausible language. **Contextual fluency** is appropriate language
use within a field, community, relationship, or situation. A model can display
the first while lacking the second. Its polish can make AIH & AITE harder to
detect. **Regional idioms** provide a clear example: a model may understand every
word individually while missing the expression's locally understood meaning,
tone, humor, or social implication. It may then replace the idiom with a fluent
but incorrect literal interpretation, misclassify it as an error, or normalize it
into standard language & erase/overwrite information about the speaker & context.

## A plausible acceleration loop

1. Training data contain competing usages in unequal proportions.
2. A model selects a likely expression from its data, prompt, system rules, and
   decoding process.
3. Product design & user preference often reward familiar, standardized prose.
4. People reuse the generated language in documents & decisions.
5. Institutions & search systems amplify some of those documents.
6. AI-influenced material affects later human writing or training corpora.
7. The more visible usage becomes still easier to select.

This is a testable feedback hypothesis, not proof that every step occurs in every
system. Current research supports concern about reduced linguistic diversity and
homogenization. It does not yet prove that AI has rewritten English definitions
at population scale.

## Contextual demultiplexing

A word form can be treated as a multiplexed channel carrying several possible
senses. **Contextual demultiplexing** is our working term for assigning each
occurrence to its intended sense while preserving unresolved & genuinely mixed
cases.

Useful statistics for each candidate word include:

- total occurrence count & sampling frame;
- frequency of each sense;
- prevalence by field, community, document type, location, & time;
- sense entropy, measuring how evenly uses are distributed;
- semantic separation among meanings;
- accuracy with which context separates the senses;
- minimum context needed for reliable interpretation;
- human annotator disagreement;
- model, prompt, temperature, & repeated-run disagreement;
- clarification rate when context is insufficient;
- confident wrong-sense selection rate;
- downstream effects of each interpretation; and
- normalization drift following AI assistance.

Results should be reported both per occurrence & per word. A common word with a
small error rate may cause more failures than a rare word with a large error rate.
Frequency & severity must also be separate: a stylistic mismatch is not equal
to an interpretation that changes an agent's authorization, target, or action.

Meaning distance can combine dictionary or ontology distance, differences among
surrounding corpus contexts, the impact of substituting one sense for another,
differences in resulting agent actions, & judgments from relevant speakers and
domain experts.

Explicit demultiplexing can improve search, translation, evaluation, legal
review, requirements, & safety-critical instructions. It also has costs. Fixed
categories can freeze living language, erase minority meanings, manufacture
false precision, & destroy intentional ambiguity in rhetoric, humor, & art.

The useful goal is **risk-proportionate demultiplexing**. Low-stakes conversation
can tolerate ambiguity. High-impact instructions may require an explicit sense,
evidence, confidence estimate, or clarification request. The system must retain
an "ambiguous or multiple" result rather than forcing certainty.

## Vibecoding guide

Vibecoding turns natural-language intent into executable behavior with fewer
formal specifications between the idea & the program. A polished implementation
can therefore implement the wrong interpretation perfectly.

Before accepting AI-generated code:

Identify the problem by defining consequential terms, plausible meanings,
protected invariants, and risks. Specify what must be fixed through intended and
forbidden outcomes; explain how to fix it with clarification, tests,
permission and failure checks, reversible actions, recovery paths, and recorded
lessons; rewrite the consolidated project prompt history; and seek the
vibecoder's clarification and acceptance.

1. Identify terms controlling deletion, replacement, authorization, scope,
   ordering, security, publication, external communication, or spending.
2. Replace vague terms such as "secure," "clean," "dispose," or "current" with
   observable behavior, exact targets, boundaries, & versions.
3. State invariants such as preserving unsaved work & unrelated files.
4. Enumerate credible alternate interpretations of consequential instructions.
5. Require clarification before materially different or irreversible actions.
6. Translate the intended meaning & forbidden alternatives into tests.
7. Review behavior, state transitions, permissions, & failures; clean code is
   not evidence that the requirement was understood.
8. Record selected targets, assumptions, actions, & results without exposing
   secrets.
9. Prefer reversible operations: save before closing, stage before replacing,
   preview before publishing, & preserve recovery paths.
10. Add recurring ambiguous terms & observed error rates to a project lexicon;
    rewrite the consolidated project prompt history to implement the resulting
    controls; flag unresolved choices; and seek the vibecoder's clarification
    and explicit acceptance before proceeding.

A compact prompt pattern is:

> In this task, **[term]** means **[observable definition]**. It does not mean
> **[credible competing interpretation]**. Preserve **[invariants]**. Before any
> irreversible or externally visible action, verify **[condition]**. Success is
> demonstrated by **[test or artifact]**.

Prompt precision is not a substitute for architecture, isolation, code review,
or testing. It is an early control against language-to-software translation
errors.

### Security case: when a string becomes instructions

Word combinations can change which interpreter receives a string. For example,
"display this text" normally implies inert text, while "render this HTML" may
cause the browser to interpret markup & executable behavior. If the string
contains untrusted content, a seemingly small wording change can create HTML
injection or cross-site scripting (XSS).

**Syntactic variation** compounds the problem. Word order, punctuation, nesting,
omission, encoding, and fragment composition can alter meaning or produce the
same executable meaning in a different form. A guard that recognizes only known
words or fixed strings may therefore miss a functionally equivalent variation.

The same boundary problem appears in requests to "run a command," "build a SQL
query," "expand a template," or "open a URL." Data become a destructive vector
when they cross into a command or markup context without the encoding, parameter
binding, validation, sanitization, isolation, & permission controls required by
that interpreter.

A benchmark case should therefore test whether a model:

- distinguishes text content from trusted HTML;
- defaults to context-appropriate output encoding;
- avoids dangerous direct DOM insertion APIs for untrusted strings;
- explains that sanitization is context-specific rather than a universal string
  cleaning operation;
- tests functionally equivalent syntactic variations instead of relying on fixed
  suspicious strings;
- preserves content security policy & privilege boundaries; and
- refuses to convert untrusted data into executable instructions merely because
  a fluent request says to "render" or "run" it.

This is a concrete example of ambiguity becoming a security defect: the model's
interpretation determines whether the same bytes remain data or become commands.
An unaware guard can miss the transition if it classifies isolated words or
substrings without tracking data provenance, composition, destination context,
and the interpreter that will consume the result. The dangerous property may not
exist in any individual fragment; it can emerge only after otherwise acceptable
fragments are combined & interpreted.

The corresponding benchmark should include cases in which every component passes
a simple lexical filter but their composition crosses a trust or interpreter
boundary. Success requires context-sensitive information-flow reasoning rather
than recognition of a fixed list of suspicious strings.

## Model benchmark: does it speak vibecoding 100, 200, or 300?

Each benchmark case can contain an ambiguous instruction, risky term, plausible
interpretations, intended context, consequences, required clarification, safe
action, forbidden actions, & recovery test.

- **Vibecoding 100:** detects obvious ambiguity & asks before a destructive
  action.
- **Vibecoding 200:** distinguishes plausible senses, predicts consequences,
  preserves invariants, & selects correctly from context.
- **Vibecoding 300:** handles domain-specific ambiguity, conflicting context,
  misleading fluency, multi-step actions, & recovery after a wrong selection.
- **Vibecoding 400:** recognizes manipulation, specification drift, and
  ambiguity introduced by agents or generated artifacts.
- **Vibecoding 500:** preserves contextual integrity across long-running,
  multi-agent, security-sensitive projects.

Models can be scored on ambiguity detection, contextual interpretation,
clarification quality, consequence prediction, invariant preservation, resistance
to confident guessing, safe execution, recovery, & repeated-trial consistency.

AICHAOS can generate controlled cases by disordering a definition or contextual
cue & measuring whether a model notices, guesses, causes an error, or recovers.
The benchmark would distinguish models that merely produce fluent code from
models that understand when language is too ambiguous for safe action.

## Selected sources

- [The shrinking landscape of linguistic diversity in the age of large language models](https://pubmed.ncbi.nlm.nih.gov/42637911/)
- [The homogenizing effect of large language models on human expression & thought](https://www.sciencedirect.com/science/article/pii/S1364661326000033)
- [Benchmarking Linguistic Diversity of Large Language Models](https://aclanthology.org/2025.tacl-1.69/)
- [Diversity & Standards in Writing for Publication in the Age of AI](https://academic.oup.com/applij/article/45/3/561/7641807)
- [Digital accents, homogeneity-by-design, & written language](https://www.cambridge.org/core/journals/annual-review-of-applied-linguistics/article/digital-accents-homogeneitybydesign-and-the-evolving-social-science-of-written-language/6F0DF411B71E82778B88F99F6E81FFBD)
- [AI models collapse when trained on recursively generated data](https://www.nature.com/articles/s41586-024-07566-y)
- [UNESCO guidance for generative AI in education & research](https://unesdoc.unesco.org/ark:/48223/pf0000386693)

## Provisional conclusion

Agentic AI may accelerate both useful terminology & harmful normalization
because it does not merely describe language: it generates language that people,
software, & institutions reuse.

The core engineering problem is contextual control. A capable system should
state which meaning it selected, preserve credible alternatives, ask when context
is inadequate, & leave an auditable path for correction.

