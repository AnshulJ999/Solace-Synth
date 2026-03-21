/* ============================================================================
 * Solace Synth — Preset Browser
 *
 * Manages preset browsing, saving, renaming, and deletion.
 * Wires into the existing preset bar (#preset-dropdown, #btn-preset-prev/next)
 * and management buttons (#btn-preset-save, etc.).
 *
 * All state lives in C++ (PresetManager). JS only reflects and requests.
 * ============================================================================ */

class PresetBrowser {
    constructor () {
        // DOM references
        this.presetName     = document.getElementById("preset-name");
        this.dropdown       = document.getElementById("preset-dropdown");
        this.btnPrev        = document.getElementById("btn-preset-prev");
        this.btnNext        = document.getElementById("btn-preset-next");
        this.btnSave        = document.getElementById("btn-preset-save");
        this.btnSaveAs      = document.getElementById("btn-preset-save-as");
        this.btnRename      = document.getElementById("btn-preset-rename");
        this.btnDelete      = document.getElementById("btn-preset-delete");
        this.modal          = document.getElementById("preset-modal");
        this.modalOverlay   = document.getElementById("preset-modal-overlay");

        // State
        this.presetList     = [];       // [{name, author, isFactory, index}]
        this.currentIndex   = 0;
        this.currentIsFactory = true;
        this.menuOpen       = false;

        this._bindEvents();
        this._bindBridge();
    }

    // ========================================================================
    // Event binding
    // ========================================================================
    _bindEvents () {
        // Arrow buttons
        this.btnPrev?.addEventListener("click", () => SolaceBridge.prevPreset());
        this.btnNext?.addEventListener("click", () => SolaceBridge.nextPreset());

        // Dropdown click — toggle hierarchical menu
        this.dropdown?.addEventListener("click", (e) => {
            e.stopPropagation();
            this.menuOpen ? this._closeMenu() : this._openMenu();
        });

        // Management buttons
        this.btnSave?.addEventListener("click", () => this._handleSave());
        this.btnSaveAs?.addEventListener("click", () => this._showSaveAsModal());
        this.btnRename?.addEventListener("click", () => this._showRenameModal());
        this.btnDelete?.addEventListener("click", () => this._showDeleteConfirm());

        // Close menu on outside click
        document.addEventListener("click", () => this._closeMenu());

        // Close menu/modal on Escape
        document.addEventListener("keydown", (e) => {
            if (e.key === "Escape") {
                this._closeMenu();
                this._closeModal();
            }
        });
    }

    _bindBridge () {
        SolaceBridge.onPresetListChanged((list) => {
            if (Array.isArray(list)) {
                this.presetList = list;
            }
        });

        SolaceBridge.onCurrentPresetChanged((data) => {
            if (data) {
                this.currentIndex = data.index ?? 0;
                this.currentIsFactory = data.isFactory ?? true;
                if (this.presetName) {
                    this.presetName.textContent = data.name ?? "Init";
                }
                this._updateManagementButtons();
            }
        });
    }

    // ========================================================================
    // Management button state
    // ========================================================================
    _updateManagementButtons () {
        // Disable rename/delete for factory presets
        if (this.btnRename)  this.btnRename.disabled = this.currentIsFactory;
        if (this.btnDelete)  this.btnDelete.disabled = this.currentIsFactory;
    }

    // ========================================================================
    // Hierarchical dropdown menu
    // ========================================================================
    _openMenu () {
        this._closeMenu();  // clean up any existing menu

        const menu = document.createElement("div");
        menu.className = "preset-menu";
        menu.addEventListener("click", (e) => e.stopPropagation());

        // Split presets into categories: Default (standalone), Factory, User
        const defaultPreset = this.presetList.find(p => p.isFactory && p.name === "Default");
        const factory = this.presetList.filter(p => p.isFactory && p.name !== "Default");
        const user    = this.presetList.filter(p => !p.isFactory);

        // Default preset — standalone item at the top
        if (defaultPreset) {
            const defaultItem = document.createElement("div");
            defaultItem.className = "preset-menu-item preset-menu-item--default";
            if (defaultPreset.index === this.currentIndex) {
                defaultItem.classList.add("preset-menu-item--active");
            }
            defaultItem.textContent = "Default";
            defaultItem.addEventListener("click", () => {
                SolaceBridge.loadPreset(defaultPreset.index);
                this._closeMenu();
            });
            menu.appendChild(defaultItem);
        }

        // Factory category
        if (factory.length > 0) {
            const catItem = this._createCategoryItem("Factory", factory);
            menu.appendChild(catItem);
        }

        // User category
        const userItem = this._createCategoryItem("User", user);
        menu.appendChild(userItem);

        // Position relative to dropdown
        const rect = this.dropdown.getBoundingClientRect();
        menu.style.top  = rect.bottom + "px";
        menu.style.left = rect.left + "px";
        menu.style.minWidth = rect.width + "px";

        document.body.appendChild(menu);
        this._activeMenu = menu;
        this.menuOpen = true;
    }

    _createCategoryItem (label, presets) {
        const item = document.createElement("div");
        item.className = "preset-menu-category";

        const labelEl = document.createElement("div");
        labelEl.className = "preset-menu-category-label";
        labelEl.textContent = label;

        const arrow = document.createElement("span");
        arrow.className = "preset-menu-arrow";
        arrow.textContent = "\u25B6";  // right triangle
        labelEl.appendChild(arrow);

        item.appendChild(labelEl);

        // Submenu
        const submenu = document.createElement("div");
        submenu.className = "preset-menu-submenu";

        if (presets.length === 0) {
            const empty = document.createElement("div");
            empty.className = "preset-menu-item preset-menu-item--empty";
            empty.textContent = "(empty)";
            submenu.appendChild(empty);
        } else {
            for (const preset of presets) {
                const presetItem = document.createElement("div");
                presetItem.className = "preset-menu-item";
                if (preset.index === this.currentIndex) {
                    presetItem.classList.add("preset-menu-item--active");
                }
                presetItem.textContent = preset.name;
                presetItem.addEventListener("click", () => {
                    SolaceBridge.loadPreset(preset.index);
                    this._closeMenu();
                });
                submenu.appendChild(presetItem);
            }
        }

        item.appendChild(submenu);

        // Show submenu on hover
        item.addEventListener("mouseenter", () => {
            submenu.classList.add("preset-menu-submenu--visible");
            // Position submenu to the right of category item
            const itemRect = item.getBoundingClientRect();
            submenu.style.top  = itemRect.top + "px";
            submenu.style.left = itemRect.right + "px";

            // If submenu goes off-screen right, flip to left
            requestAnimationFrame(() => {
                const subRect = submenu.getBoundingClientRect();
                if (subRect.right > window.innerWidth) {
                    submenu.style.left = (itemRect.left - subRect.width) + "px";
                }
                // If goes off-screen bottom, shift up
                if (subRect.bottom > window.innerHeight) {
                    submenu.style.top = Math.max(0, window.innerHeight - subRect.height) + "px";
                }
            });
        });

        item.addEventListener("mouseleave", () => {
            submenu.classList.remove("preset-menu-submenu--visible");
        });

        return item;
    }

    _closeMenu () {
        if (this._activeMenu) {
            this._activeMenu.remove();
            this._activeMenu = null;
        }
        this.menuOpen = false;
    }

    // ========================================================================
    // Save
    // ========================================================================
    _handleSave () {
        if (this.currentIsFactory) {
            // Factory preset: act as Save As
            this._showSaveAsModal();
            return;
        }

        // User preset: overwrite current
        const currentPreset = this.presetList.find(p => p.index === this.currentIndex);
        if (currentPreset) {
            SolaceBridge.savePreset(currentPreset.name);
        }
    }

    // ========================================================================
    // Modals
    // ========================================================================
    _showModal (title, defaultValue, confirmLabel, onConfirm) {
        if (!this.modal || !this.modalOverlay) return;

        const modalTitle  = this.modal.querySelector(".preset-modal-title");
        const modalInput  = this.modal.querySelector(".preset-modal-input");
        const modalCancel = this.modal.querySelector(".preset-modal-cancel");
        const modalOk     = this.modal.querySelector(".preset-modal-ok");
        const modalError  = this.modal.querySelector(".preset-modal-error");

        if (modalTitle)  modalTitle.textContent = title;
        if (modalInput)  modalInput.value = defaultValue;
        if (modalOk)     modalOk.textContent = confirmLabel;
        if (modalError)  modalError.textContent = "";

        this.modalOverlay.classList.add("visible");
        this.modal.classList.add("visible");

        // Focus input
        requestAnimationFrame(() => modalInput?.focus());

        // Clean up old listeners
        const newCancel = modalCancel?.cloneNode(true);
        const newOk     = modalOk?.cloneNode(true);
        modalCancel?.replaceWith(newCancel);
        modalOk?.replaceWith(newOk);

        newCancel?.addEventListener("click", () => this._closeModal());
        newOk?.addEventListener("click", () => {
            const val = modalInput?.value?.trim();
            if (!val) {
                if (modalError) modalError.textContent = "Name cannot be empty.";
                return;
            }
            onConfirm(val);
        });

        // Enter key submits
        modalInput?.addEventListener("keydown", (e) => {
            if (e.key === "Enter") {
                e.preventDefault();
                newOk?.click();
            }
        });
    }

    _closeModal () {
        this.modalOverlay?.classList.remove("visible");
        this.modal?.classList.remove("visible");

        // Restore input field visibility (delete confirm hides it)
        const modalInput = this.modal?.querySelector(".preset-modal-input");
        if (modalInput) modalInput.style.display = "";
    }

    _showSaveAsModal () {
        const currentPreset = this.presetList.find(p => p.index === this.currentIndex);
        const defaultName = currentPreset ? currentPreset.name : "";

        this._showModal("Save Preset As", defaultName, "Save", async (name) => {
            const result = await SolaceBridge.saveAsPreset(name);
            if (result) {
                this._closeModal();
            } else {
                const err = this.modal?.querySelector(".preset-modal-error");
                if (err) err.textContent = "Failed to save preset.";
            }
        });
    }

    _showRenameModal () {
        if (this.currentIsFactory) return;

        const currentPreset = this.presetList.find(p => p.index === this.currentIndex);
        if (!currentPreset) return;

        this._showModal("Rename Preset", currentPreset.name, "Rename", async (name) => {
            const result = await SolaceBridge.renamePreset(this.currentIndex, name);
            if (result) {
                this._closeModal();
            } else {
                const err = this.modal?.querySelector(".preset-modal-error");
                if (err) err.textContent = "Failed to rename preset.";
            }
        });
    }

    _showDeleteConfirm () {
        if (this.currentIsFactory) return;

        const currentPreset = this.presetList.find(p => p.index === this.currentIndex);
        if (!currentPreset) return;

        if (!this.modal || !this.modalOverlay) return;

        const modalTitle  = this.modal.querySelector(".preset-modal-title");
        const modalInput  = this.modal.querySelector(".preset-modal-input");
        const modalCancel = this.modal.querySelector(".preset-modal-cancel");
        const modalOk     = this.modal.querySelector(".preset-modal-ok");
        const modalError  = this.modal.querySelector(".preset-modal-error");

        if (modalTitle)  modalTitle.textContent = `Delete "${currentPreset.name}"?`;
        if (modalInput)  modalInput.style.display = "none";
        if (modalOk)     modalOk.textContent = "Delete";
        if (modalError)  modalError.textContent = "This cannot be undone.";

        this.modalOverlay.classList.add("visible");
        this.modal.classList.add("visible");

        const newCancel = modalCancel?.cloneNode(true);
        const newOk     = modalOk?.cloneNode(true);
        modalCancel?.replaceWith(newCancel);
        modalOk?.replaceWith(newOk);

        newCancel?.addEventListener("click", () => {
            if (modalInput) modalInput.style.display = "";
            this._closeModal();
        });

        newOk?.addEventListener("click", async () => {
            await SolaceBridge.deletePreset(this.currentIndex);
            if (modalInput) modalInput.style.display = "";
            this._closeModal();
        });
    }
}
