# 115 System Throey
## Lab session: ch.4  Building Classic Agent Paradigms
---

### Classic architectural paradigms
* **Reasoning and Acting (ReAct)**: think while acting and dynamically adjust.

* **Plan-and-Solve**: think before  acting

* **Reflection**: act-reflect-refine, optimizing results through self-criticism and correction.

#### ReAct
Thought -> Action -> Observation loop

* **Thought (Thinking)**: It analyzes the current situation, decomposes tasks, formulates the next plan, or reflects on the results of the previous step.

* **Action (Acting)**: This is the specific action the agent decides to take, usually calling an external tool, such as Search['Huawei's latest phone'].

* **Observation (Observing)**: This is the result returned from the external tool after executing the Action, such as a summary of search results or an API return value.

The process can be expressed as:
$$
(th_t, a_t)=\pi (q, (a_1,o_1),(a_2,o_2),...(a_{t-1}, o_{t-1}))
$$ At each time step *t*, the agent’s policy $\pi$ generates the current thought $th_t$ and action $a_t$ based on the initial question $q$ and the historical action-observation pairs $((a_1,o_1),(a_2,o_2),...(a_{t-1}, o_{t-1}))$.
$$
o_t=T(a_t)
$$Observation result is the output of tool $T$ in the environment executing action $a_t$ 
<center>

![ReAct architecture](./images/ReAct.png "ReAct architecture")
Figure 1. *ReAct workflow*
</center>
ReAct agent are capable of using external tools. A well-defined tool should contain the following three core elements:

* Name: A concise, unique identifier for the agent to call in Action, such as Search.

* Description: A clear natural language description explaining the purpose of this tool. This is the most critical part of the entire mechanism because the large language model will rely on this description to determine when to use which tool.

* Execution Logic: The function or method that actually performs the task.

##### ReAct: Pros and Limitations 
* Pros
    - High Interpretability: agent’s mental journey is visible
    - Dynamic Planning and Error Correction: take one step, look one step. If search result unsatisfying, correct search terms next step.
    - Tool Synergy: LLMs for planning and reasoning, tools for searching and calculating. Help in LLMs in knowledge timeliness and computational accuracy. 

* Limitations 
    - Rely on LLM’s capability: since LLM is in charge of thought stage, may wrong planning 
    - Execution Efficiency Issues: step-by-step requires multiple LLM calls accompanied by network latency.
    - Prompt Fragility: change in prompt template, may affect LLM behavior 
    - May Fall into Local Optima:  lacks global, long-term plan, may choose a path that seems correct in the short term but is not optimal in the long run

##### ReAct: Debugging Techniques
* Check Complete Prompt: Before each LLM call, print out the final formatted complete prompt containing all history. (trace the source of LLM decisions.)

* Analyze Raw Output: When output parsing fails (e.g., regular expressions didn't match Action), print out the unprocessed text returned by the LLM.

* Verify Tool Input and Output

* Adjust Examples in Prompt (Few-shot Prompting): add complete successful "Thought-Action-Observation" cases in the prompt to guide the model.

* Try Different Models or Parameters
##### Suitable tasks for ReAct
- Tasks requiring external knowledge: Such as querying real-time information (weather, news, stock prices), searching for knowledge in professional domains, etc.

- Tasks requiring precise calculations: Delegating mathematical problems to calculator tools to avoid LLM calculation errors.

- Tasks requiring API interaction: Such as operating databases, calling a service's API to complete specific functions.

##### ReAct Implementation
    Tool 'Search' registered.
    Tool 'Calculator' registered.
    --- Step 1 ---
    🧠 Calling gemini-3.5-flash-lite model...
    ✅ Large language model response successful:
    Thought: I need to check the current weather in Hsinchu. I will use the Search tool to find this information.
    Action: Search[weather in Hsinchu]
    Thought: I need to check the current weather in Hsinchu. I will use the Search tool to find this information.
    🎬 Action: Search[weather in Hsinchu]
    🔍 Executing [SerpApi] web search: weather in Hsinchu
    👀 Observation: [1] 天气网
    官方权威发布天气预报,逐三小时天气预报,提供天气预报查询一周,天气预报15天查询,天气预报40天查询,天气资讯,空气质量,生活指数,旅游出行,交通天气等查询服务.

    [2] Weather Merit Badge
    View current Weather Merit Badge requirements and resources from the official Scouting America Merit Badge Hub.

    [3] National Forecast - Met Éireann - The Irish Meteorological ...
    Highest temperatures of 14 to 18 degrees with mostly light to moderate west to southwest winds, freshening in the west and northwest later. National Outlook.
    --- Step 2 ---
    🧠 Calling gemini-3.5-flash-lite model...
    ✅ Large language model response successful:
    Thought: The previous search result did not give specific current weather details for Hsinchu. I need to search more specifically for the current weather in Hsinchu.
    Action: Search[current weather in Hsinchu Taiwan]
    Thought: The previous search result did not give specific current weather details for Hsinchu. I need to search more specifically for the current weather in Hsinchu.
    🎬 Action: Search[current weather in Hsinchu Taiwan]
    🔍 Executing [SerpApi] web search: current weather in Hsinchu Taiwan
    👀 Observation: [1] Hsinchu City, Hsinchu City, Taiwan Weather Forecast
    Today's Weather. Thu, Sep 17. Plenty of sunshine; a steady breeze in the afternoon Hi: 90°. Tonight: Mainly clear Lo: 76° · Current Weather. 7:31 AM. 75°F.

    [2] Hsinchu City - Cities/Counties Forecast | Central Weather ...
    Hsinchu City ; Tonight · 24 - 2775 - 81 · Comfortable ; Tomorrow · 24 - 2975 - 84 · Comfortable ~ Hot ; Tomorrow Night · 25 - 2877 - 82 · Comfortable.

    [3] Hourly Weather Forecast
    A few passing clouds, otherwise generally sunny. High 88F. Winds NE at 15 to 25 mph. icon. Tomorrow 09/16.
    --- Step 3 ---
    🧠 Calling gemini-3.5-flash-lite model...
    ✅ Large language model response successful:
    Thought: I have found the current weather information for Hsinchu, which is currently sunny/clear with temperatures around 75°F (approx. 24-27°C) and a steady breeze or comfortable conditions. I can now provide the final answer.
    Action: Finish[The weather in Hsinchu is currently sunny and clear, with temperatures around 75°F (approx. 24°C) and comfortable conditions.]
    Thought: I have found the current weather information for Hsinchu, which is currently sunny/clear with temperatures around 75°F (approx. 24-27°C) and a steady breeze or comfortable conditions. I can now provide the final answer.
    🎉 Final Answer: The weather in Hsinchu is currently sunny and clear, with temperatures around 75°F (approx. 24°C) and comfortable conditions.

#### Plan-and Solve
Plan first, then Solve. 
To solve the problem that chain-of-thought easily "goes off track" when handling multi-steps

1. **Planning Phase**: decompose the problem and formulate a clear, step-by-step action plan.

2. **Solving Phase**: obtaining the complete plan, strictly execute according to the steps in the plan, one by one.

The two-stage process can be express as:
$$
P=\pi_{plan}(q)
$$The planning model $\pi_{plan}$  generates a plan $P=(p_1, p_2, ..., p_n)$ containing $n$ steps based on the original question $q$
$$
s_i=\pi_{solve}(q, P, (s_1, s_2, ..., s_{i-1}))
$$Execution model $\pi_{solve}$  outputs solution $𝑠_𝑖$ which depend on the original question $q$, the complete plan *P*, and the execution results of all previous steps $(s_1, s_2, ..., s_{i-1})$:

<center>

![ReAct architecture](./images/plan-and-solve.png "ReAct architecture")
Figure 2. *Plan-and-Solve workflow*
</center>

##### Suitable tasks for Plan-and Solve
* Multi-step math word problems: Need to first list calculation steps, then solve one by one.

* Report writing integrating multiple information sources: Need to first plan the report structure (introduction, data source A, data source B, summary).

* Code generation tasks: Need to first conceive the structure of functions, classes, and modules.

##### Plan-and-Solve Implementation
    --- Starting to Process Question ---
    Question: A factory produced 25 toy cars on Monday. The number of toy cars produced on Tuesday was twice that of Monday. The number produced on Wednesday was 15 fewer than Tuesday. How many toy cars were produced in total over these three days?
    --- Generating Plan ---
    🧠 Calling gemini-3.5-flash-lite model...
    ✅ Large language model response successful:
    ```python
    [
        "Identify the number of toy cars produced on Monday (25).",
        "Calculate the number of toy cars produced on Tuesday by multiplying Monday's production by 2.",
        "Calculate the number of toy cars produced on Wednesday by subtracting 15 from Tuesday's production.",
        "Calculate the total number of toy cars produced over the three days by adding the production of Monday, Tuesday, and Wednesday."
    ]
    ```
    ✅ Plan Generated:
    ```python
    [
        "Identify the number of toy cars produced on Monday (25).",
        "Calculate the number of toy cars produced on Tuesday by multiplying Monday's production by 2.",
        "Calculate the number of toy cars produced on Wednesday by subtracting 15 from Tuesday's production.",
        "Calculate the total number of toy cars produced over the three days by adding the production of Monday, Tuesday, and Wednesday."
    ]
    ```

    --- Executing Plan ---

    -> Executing step 1/4: Identify the number of toy cars produced on Monday (25).
    🧠 Calling gemini-3.5-flash-lite model...
    ✅ Large language model response successful:
    25
    ✅ Step 1 completed, result: 25

    -> Executing step 2/4: Calculate the number of toy cars produced on Tuesday by multiplying Monday's production by 2.
    🧠 Calling gemini-3.5-flash-lite model...
    ✅ Large language model response successful:
    50
    ✅ Step 2 completed, result: 50

    -> Executing step 3/4: Calculate the number of toy cars produced on Wednesday by subtracting 15 from Tuesday's production.
    🧠 Calling gemini-3.5-flash-lite model...
    ✅ Large language model response successful:
    35
    ✅ Step 3 completed, result: 35

    -> Executing step 4/4: Calculate the total number of toy cars produced over the three days by adding the production of Monday, Tuesday, and Wednesday.
    🧠 Calling gemini-3.5-flash-lite model...
    ✅ Large language model response successful:
    110
    ✅ Step 4 completed, result: 110

    --- Task Completed ---
    Final Answer: 110

#### Reflection
Execute -> Reflect -> Refine

1. **Execution**: generate first draft that completes the task (ReAct or Plan-and-Solve). 

2. **Reflection**: a "reviewer” examines the "first draft" from multiple dimensions, such as:
    - Factual Errors: Is there content that contradicts common sense or known facts?
    - Logical Flaws: Are there inconsistencies or contradictions in the reasoning process?
    - Efficiency Issues: Is there a more direct, more concise path to complete the task?
    - Missing Information: Are some key constraints or aspects of the problem overlooked? Based on the evaluation, it generates structured Feedback, pointing out specific problems and improvement suggestions.

3. **Refinement**: based on  the "first draft" and "feedback" generates a more complete "revised draft."

The  iterative optimization process can be expressed as:
$$
F_i=\pi_{reflect}(\text{task}, O_i)
$$Reflection model $\pi_{reflect}$  generates feedback $𝐹_𝑖$ for output produced by the 𝑖-th iteration $O_𝑖$.
$$
O_{i+1}=\pi_{refine}(\text{task}, O_i, F_i)
$$The refinement model $\pi_{refine}$  combines the original task, the previous version's output, and feedback to generate a new version's output $O_{𝑖+1}$

##### Reflection: pros
* Pros
    - Internal error correction loop, thus able to correct higher-level logical and strategic errors.
    - A continuous optimization process, significantly improving the final success rate and answer quality for complex tasks.
    - A temporary "short-term memory" for the agent allowing the agent emembers how it iterated from a flawed first draft to the final version. This memory system can also be multimodal, allowing the agent to reflect on and revise outputs beyond text (such as code, images, etc.), 

### Selection Strategy for Different Agent Loops
<center>

| When your task... | Preferred Choice | Core Reason |
| :--- | :--- | :--- |
| Is full of uncertainty and requires interaction with external APIs or web pages | **ReAct** | It can dynamically adjust its path based on real-time feedback. |
| Has a clear logical path, focusing on internal reasoning and step-by-step decomposition | **Plan-and-Solve** | It provides a stable, structured execution workflow. |
| Has extreme requirements for the quality and reliability of the final result | **Reflection** | Through iterative optimization, it can elevate a "satisfactory" answer to "excellent". |
</center>

### Exercise
 
1. This chapter introduced three classic agent paradigms:      ReAct, Plan-and-Solve, and Reflection. Please analyze:

    - Can these three paradigms be combined? If so, please try to design a hybrid paradigm agent architecture and explain its applicable scenarios.
    
        **Sure, we can make use of ReAct’s dynamically adjusting advantage combined with Plan-and-Solve to plan a global path. The scenario could be driving a car from point A to point B.**


2. In the ReAct implementation in Section 4.2, we used regular expressions to parse the large language model's output (such as Thought and Action). Please consider:

    - What potential fragilities exist in the current parsing method? Under what circumstances might it fail?
    <center>

    ![ReAct architecture](./images/regular_expression_fragile.png "ReAct architecture")
    Figure 4. *ReAct workflow*
    </center>
    Besides regular expressions, what are some more robust output parsing solutions?
    <center>

    ![ReAct architecture](./images/pydantic.png "ReAct architecture")
    Figure 5. *pydantic*
    </center>

    modifying the code to use pydantic class:
    <center>

    ![ReAct architecture](./images/ReAct_pydantic.png "ReAct architecture")
    Figure 6. *ReAct using pydantic class*
    </center>

4. The Plan-and-Solve paradigm decomposes tasks into two stages: "planning" and "execution." Please analyze in depth:

    - Try designing a "hierarchical planning" system: first generate a high-level abstract plan, then generate detailed sub-plans for each high-level step. What advantages does this design have?
    **The advantage of this design is when a certain plan is too hard to complete in one step, this method provide llm how to dealt with it** 
    <center>

    ![ReAct architecture](./images/plan_and_solve_replanner.png "ReAct architecture")
    Figure 7. *Conceptual Architecture Sketch*
    </center>
   
Coding Implementation: 

    --- Starting to Process Question ---
    Question: 總共有25匹馬，5個跑道。你現在沒有任何計時的方法測試每匹馬的實際跑速。請問最少需要讓他們比賽幾次，才能分出25匹馬中的前3名?
    --- Generating Plan ---
    🧠 Calling gemini-3.5-flash-lite model...
    ✅ Large language model response successful:
    ```python
    ["組織全體馬匹進行分組初賽以覆蓋所有候選者", "篩選並安排各組優勝者進行頂尖對決以確定相對排名", "綜合初賽與決賽結果推導並確認最終前三名"]
    ```
    ✅ Plan Generated:
    ```python
    ["組織全體馬匹進行分組初賽以覆蓋所有候選者", "篩選並安排各組優勝者進行頂尖對決以確定相對排名", "綜合初賽與決賽結果推導並確認最終前三名"]
    ```
    --- Generating sub-Plan ---
    🧠 Calling gemini-3.5-flash-lite model...
    ✅ Large language model response successful:
    ```python
    ["制定馬匹分組初賽賽程與場地規則", "組織全體候選馬匹進行分組初賽並記錄成績", "根據初賽成績篩選出各組優勝馬匹", "安排晉級馬匹進行頂尖對決決賽", "評定決賽表現並結合初賽數據推導相對排名", "綜合初賽與決賽結果確認並公佈最終前三名"]
    ```
    ✅ sub-Plan Generated:
    ```python
    ["制定馬匹分組初賽賽程與場地規則", "組織全體候選馬匹進行分組初賽並記錄成績", "根據初賽成績篩選出各組優勝馬匹", "安排晉級馬匹進行頂尖對決決賽", "評定決賽表現並結合初賽數據推導相對排名", "綜合初賽與決賽結果確認並公佈最終前三名"]
    ```

    --- Executing Plan ---

    -> Executing step 1/6: 制定馬匹分組初賽賽程與場地規則
    🧠 Calling gemini-3.5-flash-lite model...
    ✅ Large language model response successful:
    將25匹馬隨機分為5組（每組5匹），分別進行5場初賽，每場比賽記錄各馬匹的到達順序與相對名次。
    ✅ Step 1 completed, result: 將25匹馬隨機分為5組（每組5匹），分別進行5場初賽，每場比賽記錄各馬匹的到達順序與相對名次。

    -> Executing step 2/6: 組織全體候選馬匹進行分組初賽並記錄成績
    🧠 Calling gemini-3.5-flash-lite model...
    ✅ Large language model response successful:
    已完成將25匹馬隨機分為5組（第1組至第5組，每組5匹），並依序組織這5組馬匹進行了5場初賽，完整記錄了每場比賽中各馬匹的到達順序與相對名次。
    ✅ Step 2 completed, result: 已完成將25匹馬隨機分為5組（第1組至第5組，每組5匹），並依序組織這5組馬匹進行了5場初賽，完整記錄了每場比賽中各馬匹的到達順序與相對名次。

    -> Executing step 3/6: 根據初賽成績篩選出各組優勝馬匹
    🧠 Calling gemini-3.5-flash-lite model...
    ✅ Large language model response successful:
    已從5場初賽中篩選出各組的前三名馬匹，總計15匹馬進入下一階段，其餘10匹馬（每組的第4、第5名及其之後因遞補關係而無緣晉級的馬匹）遭到淘汰。
    ✅ Step 3 completed, result: 已從5場初賽中篩選出各組的前三名馬匹，總計15匹馬進入下一階段，其餘10匹馬（每組的第4、第5名及其之後因遞補關係而無緣晉級的馬匹）遭到淘汰。

    -> Executing step 4/6: 安排晉級馬匹進行頂尖對決決賽
    🧠 Calling gemini-3.5-flash-lite model...
    ✅ Large language model response successful:
    讓5個組別的每組第一名（共5匹馬）進行第6場決賽，並記錄這場比賽的前三名到達順序。
    ✅ Step 4 completed, result: 讓5個組別的每組第一名（共5匹馬）進行第6場決賽，並記錄這場比賽的前三名到達順序。

    -> Executing step 5/6: 評定決賽表現並結合初賽數據推導相對排名
    🧠 Calling gemini-3.5-flash-lite model...
    ✅ Large language model response successful:
    根據第6場決賽的結果，設決賽前三名為 A1、B1、C1（假設其在決賽的名次為 A1 > B1 > C1）。此時 A1 確定為全部 25 匹馬中的第 1 名。接著結合初賽數據：只有在決賽中獲得前三名的馬匹所屬的初賽組別中，才有可能產生總排名的第 2 名或第 3 名。具體而言，只需考慮 A1 所在組別的第 2、3 名，B1 所在組別的第 1、2 名，以及 C1 所在組別的第 1 名（共 5 匹馬：A2, A3, B1, B2, C1），透過邏輯推導排除不可能進入前三名的馬匹，縮小最終候選範圍。
    ✅ Step 5 completed, result: 根據第6場決賽的結果，設決賽前三名為 A1、B1、C1（假設其在決賽的名次為 A1 > B1 > C1）。此時 A1 確定為全部 25 匹馬中的第 1 名。接著結合初賽數據：只有在決賽中獲得前三名的馬匹所屬的初賽組別中，才有可能產生總排名的第 2 名或第 3 名。具體而言，只需考慮 A1 所在組別的第 2、3 名，B1 所在組別的第 1、2 名，以及 C1 所在組別的第 1 名（共 5 匹馬：A2, A3, B1, B2, C1），透過邏輯推導排除不可能進入前三名的馬匹，縮小最終候選範圍。

    -> Executing step 6/6: 綜合初賽與決賽結果確認並公佈最終前三名
    🧠 Calling gemini-3.5-flash-lite model...
    ✅ Large language model response successful:
    7
    ✅ Step 6 completed, result: 7 

4. The Reflection mechanism improves output quality through the "execute-reflect-refine" loop. Please consider:
    - Suppose you want to build an "academic paper writing assistant" that can generate drafts and continuously optimize paper content. Please design a multi-dimensional Reflection mechanism that reflects and improves from multiple perspectives such as paragraph logic, method innovation, language expression, and citation standards.


5. Prompt engineering is a key technology affecting the final effect of agents. This chapter demonstrated multiple carefully designed prompt templates. Please analyze:

    - In the Reflection prompt in Section 4.4.3, we used a role setting like "you are an extremely strict code review expert." Try modifying this role setting (such as changing it to "you are an open-source project maintainer who values code readability"), observe the changes in output results, and summarize the impact of role settings on agent behavior.


    | original | modified |
    | :---: | :---: |
    | ![ReAct architecture](./images/prompt1.png "ReAct architecture") | ![圖片2](./images/prompt2.png) |


    - Adding few-shot examples to prompts can often significantly improve the model's ability to follow specific formats. Please try adding few-shot examples to one of the agents in this chapter and compare the effects.
        
        zero-shot:

            Tool 'Search' registered.
            Tool 'Calculator' registered.

            --- Available Tools ---
            - Search: A web search engine. Use this tool when you need to answer questions about current events, facts, and information not found in your knowledge base.
            - Calculator: A calculator. Use this tool when you need to answer questions about mathematical calculations.

            --- Execute Action: Search['What is VLA ---
            --- Observation ---
            Error evaluating expression 'Calculate the result of (123 + 456) * 789 / 12': invalid syntax (<unknown>, line 1)
        
        Adding few-shot examples: 

            REACT_PROMPT_TEMPLATE = """
            Please note that you are an intelligent assistant capable of calling external tools.

            Available tools are as follows:
            {tools}

            Please respond strictly in the following format:

            Thought: Your thinking process, used to analyze problems, decompose tasks, and plan the next action.
            Action: The action you decide to take, must be in one of the following formats:
            - {{tool_name}}[{{tool_input}}]`: Call an available tool.
            - Use the Calculator tool when the question requires mathematical calculation.
            - When calling Calculator, provide ONLY the mathematical expression as the tool input.
            Do NOT include words such as "Calculate", "result", "=" or "?".
            Example:
            Correct: Calculator[(123 + 456) * 789 / 12]
            Incorrect: Calculator[Calculate the result of (123 + 456) * 789 / 12 = ?]
            - `Finish[final answer]`: When you believe you have obtained the final answer.
            - When you have collected enough information to answer the user's final question, you must use `Finish[final answer]` after the Action: field to output the final answer.

            Now, please start solving the following problem:
            Question: {question}
            History: {history}
            """
        ## 
        Successful output after adding few-shot examples:

            Tool 'Search' registered.
            Tool 'Calculator' registered.
            --- Step 1 ---
            🧠 Calling gemini-3.5-flash-lite model...
            ✅ Large language model response successful:
            Thought: The user wants to calculate the mathematical expression `(123 + 456) × 789 / 12`. I need to use the Calculator tool for this.
            Action: Calculator[(123 + 456) * 789 / 12]
            Thought: The user wants to calculate the mathematical expression `(123 + 456) × 789 / 12`. I need to use the Calculator tool for this.
            🎬 Action: Calculator[(123 + 456) * 789 / 12]
            👀 Observation: 38069.25
            --- Step 2 ---
            🧠 Calling gemini-3.5-flash-lite model...
            ✅ Large language model response successful:
            Thought: The calculation has already been performed and the result is 38069.25. I can now provide the final answer.
            Action: Finish[38069.25]
            Thought: The calculation has already been performed and the result is 38069.25. I can now provide the final answer.
            🎉 Final Answer: 38069.25
