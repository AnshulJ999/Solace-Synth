# AI Agent Memory Update Protocol

**Applies to:** Any AI agent (Claude Code, Gemini, Codex, Antigravity, Jules, etc.) that edits `.agent/synth-project-memory.md`.

**The project memory is the single source of truth.** All other plan files, roadmaps, and audit docs are references. If they contradict the memory, the memory wins. If the memory is wrong, fix it — don't create a new doc that says something different.

---

## Before Editing

1. **Read the full header block** (top ~40 lines) to understand current status.
2. **Search the entire file** for all mentions of the item you're updating. Use ctrl+F / grep for the parameter name, phase number, or feature keyword.
3. **Do not create contradictions.** If you mark something COMPLETE in one place, mark it everywhere. If an item appears in both the header status notes and a phase log and the pre-release backlog, update ALL of them.

## When Editing

4. **Phase logs are frozen history.** Completed phase entries (6.1, 6.2, ... 7.5) document what happened. Do not rewrite them unless correcting a factual error. New work goes in a new section or the header status notes.
5. **Mark resolved items explicitly.** Change `⚠️ OPEN` or `[ ]` to `✅ RESOLVED` or `[x]` with a one-line resolution. Don't delete the context — future agents need to understand what was decided and why.
6. **Update the V1/V2 scope tables** when implementation diverges from the original plan. The scope should always reflect what's actually built, not the original spec.
7. **Use absolute dates**, not relative ones. "Last week" means nothing to a future agent. Write "2026-03-21".
8. **Don't duplicate content.** Before adding a new section, check if the information already exists somewhere in the file. Update the existing entry instead.

## What NOT to Do

9. **Don't rewrite the file from scratch.** Targeted edits only. The file's structure and historical entries are important context.
10. **Don't implement features based on AI peer reviews** without Anshul's explicit approval. AI reviews are input, not authority. Anshul is the sole decision-maker.
11. **Don't add items to the "Pending Nabeel" category.** Anshul handles everything. If you need a design decision, flag it for Anshul.
12. **Don't guess parameter values, enum indices, or implementation details.** Read the actual source code (`Source/`, `UI/`) to verify. The memory should reflect what the code actually does.

## After Editing

13. **Update the `Last Updated` date** in the header with today's date and a brief description of what changed.
14. **Verify no contradictions** by searching for the keywords you just edited.

---

## File Structure Reference

The memory file has this structure (maintain it):

```
Header block (status, next focus, quick notes)
───────────────────────────────────────────────
Project Overview
Project Name
Team Roles
Vision Document + UI Design tables
V1/V2 Scope (keep current with implementation)
Figma → Plugin UI Workflow (historical, rarely edited)
Bugs, Patterns & Pre-Release Checklist (actively maintained)
Technical Approach (historical, rarely edited)
APC Assessment (historical)
License Summary (historical)
Build System Details (update when build config changes)
Current Status & Next Steps (actively maintained)
  - Done checklist
  - Pre-Release Backlog
  - Spec Gaps
  - SVG Icon Status
  - Phase implementation logs (frozen after completion)
Testing Strategy
Git Workflow
Design Spec
Open Questions (keep small — move resolved items out)
Key Links
```
