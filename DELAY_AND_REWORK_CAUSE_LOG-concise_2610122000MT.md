20261013 1000MT
Life of a Vibe-Coder, contd.

VC has become an issue of recognizing & resolving AI hallucination & translation efficiency (AIH & AITE). 

Remember that you're VCing on a dev box in a dev-box env. 
Every change made to the file-structure will result in dev inefficiency 
as both the VC & their agents will have to reorient to the change. 
Some change is to be expected & is good for overall VC growth. 
A lot of change will cause serious problems.

Deploying dev code is a whole different issue than developing dev code 
BUT ideally you develop a plan for both. The more that you improve dev & deployment, 
the better your dev/deployment plan will be. 

The A#1 step is to convert prompt into math to minimize AIH & AITE 
and THEN follow a logical build procedure that eliminates agentic AI from the build. 
Then do dev in a consistent manner. 
I VC in Bash/Cpp/QT6 in Ubuntu 24 through state machine logic to bash scripts 
that implement state machines in cpp/qt6 executables. Everything is tracked & logged appropriately. 
Ultimately correcting inefficient sw-dev.

I build flowcharts into my code with AI, not AI into my code. 

# AIChessPlay Restart: Concise Delay & Rework Causes

01. The objective combined 2 projects: restoring AIChessPlay & designing a general efficient VC procedure.

02. “Efficiency” initially mixed workflow efficiency with conventional runtime & memory efficiency.

03. The original harness was too monolithic to easily pinpoint failure.

04. Bash & orchestration failures could be mistaken for model failures.

05. Logs recorded stage starts but sometimes omitted decisive outcomes.

06. Cache invalidation was too broad, causing unnecessary rework.

07. Model identity relied on filenames rather than content hashes.

08. Open-source model eligibility was not initially enforced by a clear policy.

09. SmolLM2 & TinyLlama produced genuine prefilter failures; OLMo passed qualification.

10. llama.cpp runtime-library resolution caused a tournament-path infrastructure failure.

11. Actions require clear decision authority even when VC.

12. Run the batch files manually to maintain a clear dev-chain & minimize project churn.
 
13. Dense or insufficiently isolated dev work caused additional dev rework.

14. AI Conversation-window pressure interrupted consolidation & encouraged localized logging.

15. Changing agent model & reasoning config has an unknown effect on VC efficiency.

16. Design an auto-config mode into the project so that the code auto-configures itself 
to run to completion without crashing. The user can then selectively alter the configuration & re-run in auto-config mode. 

17. We need to decide how to change a setting in terms of what is an aggressive vs a conservative change. 
Aggressive changes will make the software less stable, conservative changes will make it more stable. 
So we need to build some intelligence into the auto-tuning concept. 
In this specific case we had a comm runaway which was only made worse by increasing the related timeout. 
So how is the code detecting a runaway? We will have to build the associated classification logic into the code, 
else we will have to build an AI agent into the code and one that it performs the classification and auto-tuning correctly. 
That latter option we generally want to avoid and the former option gives us a deterministic auto-tuning capability. 
But this means for each error we need to be able to at least give a reasonable guess as to why the error occured. 
This can be done if we can tie each error down to a finite list of reasons why that error occurred. 
We would then have to develop a debugging routine for each error...
i.e. put a decision-tree into the debugging for the auto-tune to have a realistic chance to work. 
We will have to adjust the software conservatively in retrograde until the failing stage succeeds.

18. Implement a lightweight & effective handshaking protocol for inter-process communication where useful for debugging. 

19. Plan project-mods effectively between runs effectively. 
Make sure they form a complete set and then implement them all in one rev.

20. Be aware of the 90-10 rule: 90% of the work is easy, the last 10% is hard. 
Be prepared to loop through steps 16-20 repeatedly until the ingredients are well-mixed. 

21. Look for the agent to identify commonality and conflicts in the current project of interest 
and ask the user whatever questions its think are necessary to eliminate the conflicts 
and guide the project of interest towards a minimal AITE state.

22. Don't assist the agent when it overly attempts to engage in AIH and AITE.
Stand your ground.
This may be more of a problem for the commmercial cloud-based agentic AI systems
than for local agentic AI stacks. But without a functional local agentic AI stack, 
this cannot really be isolated to commercial cloud-based agentic AI systems.

23. Don't assume that the data that an agentic AI agent gives in response is valid data. 
Require the agent to establish that by demonstrating how the data was generated 
and then have the agent consider other data-generation methods and use a quorum 
to decide among them. Have the agent establish relevant PDF experimentally. 

24. Ask yes/no quesitions whenever possible. 
Ultimately use a prompt-response wrapper to qualitatively evaluate prompt-response pairs 
based on the evaluation of each side as to the quality of the prompt-response pair.
Don't give too much credence to the individual prompt-response pairs.
The point of diagnostic observations is to guide user-model interaction towards 100% effectiveness, 
therefore we must avoid endless recursion in the diagnostic observations.

25. Ultimately the vibe-coder must, at least, attempt to be as logical and complete as possible.
Assuming that an agentic AI agent will do this for the vibe-coder is a recipe for failure. 

26. Don't allow the agent that you are using for vibe-coding to run the 
software that it is producing to "smoke-test" it or otherwise to debug it. 
That has to be the vibe-coders' responsiblity or else the agent will spend 
a near-infinite amount of tokens doing this testing for the VC. 
And it will be done to the agents' standards not the VC's standards. 

27. Ultimately you want to get the agent to ask you questions about how the VC wants to implement the prompts, 
not just run off and merilly do whatever it decides to do and hand you a result.
The VC should ask the agent if it has any questions of the VC before it does anything more to change the code. 
Repeat this process between each run of the code. 
This is where an appropriate statistical evaluation of the project will contribute substantially 
to reducing AITE so it is important to have the agent generate the appropriate stastical data 
to experimentally form the appropriate PDFs to support architectual/sw-development decisions.

28. ...likewise all architectual/sw-dev decisions must be tracked over the project lifetime 
so that a verifiable cause-effect relationship can be established. 
As alwways this assumes that the agent is at least usefully competent, 
but it offloads decision-responsiblity from the agent in pursuit of the reduction of AITE.

29. In the general sense the VC needs to issue prompts similar to this one...
"when I tell you that the run is complete I want you to analyze the resultant results, 
give me a md file that discusses the aichessplay project progress to date (open it in gedit) 
prepare a project goal documentation udate Q/A file (open in gedit) before you generate your next mod 
of either the effvcmk1.sh or the effvcmk2.sh and then stop. I need to address some higher-level issues."


This list is still under dev. We do not have an adequate aichessplay build. 
The ultimate goal here is to develop an adequate local AI-stack to replace Codex "adequately". 
To do that I need an adequate VC dev process to use against a local AI stack. 

Summary: 
Work in terms of logical, documented and logged states.
Minimize dev dependency on agentic AI. 
