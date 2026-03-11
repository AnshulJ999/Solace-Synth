

"G:\GitHub\Solace-Synth\.agent\synth-project-memory.md"

This is the rolling project memory. You must always keep it updated. After each message / task, evaluate if the memory needs updating and do so automatically. If confirmation needed, ask the user. 

For memory updates, prefer appends instead of rewrites. Old/stale memory context should either be crossed-out to mark as old, or marked as resolved/done. The memory file should contain enough context any new AI can read it and instantly know where we stand.

Remember, when updating memory, you have to add actual details of what you did. You don't just 'replace the status line at the top', that's lazy. We need details of what you actually did. And don't overwrite/rewrite any useful memories or valuable context.

Coding practices: 

- Never assume, hallucinate, or guess code. Don't write code you're not sure about or confident with.
- You can use various MCPs like Exa and GitHub to fetch code context, snippets, documentation, relevant repositories, etc. This'll help guide the code properly.
- If you're ever confused or find yourself spiralling with a problem you can't reconcile, STOP, and discuss with the user. 
- Do not make any critical decisions on your own without discussion and approval from the user.
- Use relevant Skills automatically.

---

G:\GitHub\Solace-Synth\.agent\Vision-Document.md

Always read the above vision document too. This one is maintained by Nabeel and contains his vision for the project. If the plans/memories do not match the Vision, please flag any such issues and discuss with me to clarify.