

"G:\GitHub\Solace-Synth\.agent\synth-project-memory.md"

This is the rolling project memory. You must always keep it updated. After each message / task, evaluate if the memory needs updating and do so automatically. If confirmation needed, ask the user. 

For memory updates, prefer appends instead of rewrites. Old/stale memory context should either be crossed-out to mark as old, or marked as resolved/done. The memory file should contain enough context any new AI can read it and instantly know where we stand.

Coding practices: 

- Never assume, hallucinate, or guess code. Don't write code you're not sure about or confident with.
- You can use various MCPs like Exa and GitHub to fetch code context, snippets, documentation, relevant repositories, etc. This'll help guide the code properly.
- If you're ever confused or find yourself spiralling with a problem you can't reconcile, STOP, and discuss with the user. 
- Do not make any critical decisions on your own without discussion and approval from the user.
- Use relevant Skills automatically.