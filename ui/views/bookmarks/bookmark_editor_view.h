// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_EDITOR_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_EDITOR_VIEW_H_

#include <vector>

#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/strings/string16.h"
#include "chrome/browser/bookmarks/bookmark_expanded_state_tracker.h"
#include "chrome/browser/bookmarks/bookmark_model_observer.h"
#include "chrome/browser/ui/bookmarks/bookmark_editor.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/base/models/tree_node_model.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/controls/tree/tree_view_controller.h"
#include "ui/views/window/dialog_delegate.h"

namespace views {
class Label;
class LabelButton;
class MenuRunner;
class TreeView;
}

class BookmarkEditorViewTest;
class GURL;
class Menu;
class Profile;

// View that allows the user to edit a bookmark/starred URL. The user can
// change the URL, title and where the bookmark appears as well as adding
// new folders and changing the name of other folders. The editor is used for
// both editing a url bookmark, as well as editing a folder bookmark when
// created from 'Bookmark all tabs'.
//
// Edits are applied to the BookmarkModel when the user presses 'OK'.
//
// To use BookmarkEditorView invoke the static show method.

class BookmarkEditorView : public BookmarkEditor,
                           public views::ButtonListener,
                           public views::TreeViewController,
                           public views::DialogDelegateView,
                           public views::TextfieldController,
                           public views::ContextMenuController,
                           public ui::SimpleMenuModel::Delegate,
                           public BookmarkModelObserver {
 public:
  // Type of node in the tree. Public purely for testing.
  typedef ui::TreeNodeWithValue<int64> EditorNode;

  // Model for the TreeView. Trivial subclass that doesn't allow titles with
  // empty strings. Public purely for testing.
  class EditorTreeModel : public ui::TreeNodeModel<EditorNode> {
   public:
    explicit EditorTreeModel(EditorNode* root)
        : ui::TreeNodeModel<EditorNode>(root) {}

    virtual void SetTitle(ui::TreeModelNode* node,
                          const string16& title) OVERRIDE;

   private:
    DISALLOW_COPY_AND_ASSIGN(EditorTreeModel);
  };

  BookmarkEditorView(Profile* profile,
                     const BookmarkNode* parent,
                     const EditDetails& details,
                     BookmarkEditor::Configuration configuration);

  virtual ~BookmarkEditorView();

  // views::DialogDelegateView:
  virtual string16 GetDialogButtonLabel(ui::DialogButton button) const OVERRIDE;
  virtual bool IsDialogButtonEnabled(ui::DialogButton button) const OVERRIDE;
  virtual views::View* CreateExtraView() OVERRIDE;
  virtual ui::ModalType GetModalType() const OVERRIDE;
  virtual bool CanResize() const  OVERRIDE;
  virtual string16 GetWindowTitle() const  OVERRIDE;
  virtual bool Accept() OVERRIDE;

  // views::View:
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual void GetAccessibleState(ui::AccessibleViewState* state) OVERRIDE;

  // views::TreeViewController:
  virtual void OnTreeViewSelectionChanged(views::TreeView* tree_view) OVERRIDE;
  virtual bool CanEdit(views::TreeView* tree_view,
                       ui::TreeModelNode* node) OVERRIDE;

  // views::TextfieldController:
  virtual void ContentsChanged(views::Textfield* sender,
                               const string16& new_contents) OVERRIDE;
  virtual bool HandleKeyEvent(views::Textfield* sender,
                              const ui::KeyEvent& key_event) OVERRIDE;

  // views::ButtonListener:
  virtual void ButtonPressed(views::Button* sender,
                             const ui::Event& event) OVERRIDE;

  // ui::SimpleMenuModel::Delegate:
  virtual bool IsCommandIdChecked(int command_id) const OVERRIDE;
  virtual bool IsCommandIdEnabled(int command_id) const OVERRIDE;
  virtual bool GetAcceleratorForCommandId(
      int command_id,
      ui::Accelerator* accelerator) OVERRIDE;
  virtual void ExecuteCommand(int command_id, int event_flags) OVERRIDE;

  // Creates a Window and adds the BookmarkEditorView to it. When the window is
  // closed the BookmarkEditorView is deleted.
  void Show(gfx::NativeWindow parent);

  // views::ContextMenuController:
  virtual void ShowContextMenuForView(views::View* source,
                                      const gfx::Point& point,
                                      ui::MenuSourceType source_type) OVERRIDE;

 private:
  friend class BookmarkEditorViewTest;

  // Creates the necessary sub-views, configures them, adds them to the layout,
  // and requests the entries to display from the database.
  void Init();

  // BookmarkModel observer methods. Any structural change results in
  // resetting the tree model.
  virtual void Loaded(BookmarkModel* model, bool ids_reassigned) OVERRIDE {}
  virtual void BookmarkNodeMoved(BookmarkModel* model,
                                 const BookmarkNode* old_parent,
                                 int old_index,
                                 const BookmarkNode* new_parent,
                                 int new_index) OVERRIDE;
  virtual void BookmarkNodeAdded(BookmarkModel* model,
                                 const BookmarkNode* parent,
                                 int index) OVERRIDE;
  virtual void BookmarkNodeRemoved(BookmarkModel* model,
                                   const BookmarkNode* parent,
                                   int index,
                                   const BookmarkNode* node) OVERRIDE;
  virtual void BookmarkAllNodesRemoved(BookmarkModel* model) OVERRIDE;
  virtual void BookmarkNodeChanged(BookmarkModel* model,
                                   const BookmarkNode* node) OVERRIDE {}
  virtual void BookmarkNodeChildrenReordered(BookmarkModel* model,
                                             const BookmarkNode* node) OVERRIDE;
  virtual void BookmarkNodeFaviconChanged(BookmarkModel* model,
                                          const BookmarkNode* node) OVERRIDE {}

  // Resets the model of the tree and updates the various buttons appropriately.
  void Reset();

  // Expands all the nodes in the tree and selects the parent node of the
  // url we're editing or the most recent parent if the url being editted isn't
  // starred.
  void ExpandAndSelect();

  // Creates a returns the new root node. This invokes CreateNodes to do
  // the real work.
  EditorNode* CreateRootNode();

  // Adds and creates a child node in b_node for all children of bb_node that
  // are folders.
  void CreateNodes(const BookmarkNode* bb_node, EditorNode* b_node);

  // Returns the node with the specified id, or NULL if one can't be found.
  EditorNode* FindNodeWithID(BookmarkEditorView::EditorNode* node, int64 id);

  // Invokes ApplyEdits with the selected node.
  void ApplyEdits();

  // Applies the edits done by the user. |parent| gives the parent of the URL
  // being edited.
  void ApplyEdits(EditorNode* parent);

  // Recursively adds newly created folders and sets the title of nodes to
  // match the user edited title.
  //
  // bb_node gives the BookmarkNode the edits are to be applied to, with b_node
  // the source of the edits.
  //
  // If b_node == parent_b_node, parent_bb_node is set to bb_node. This is
  // used to determine the new BookmarkNode parent based on the EditorNode
  // parent.
  void ApplyNameChangesAndCreateNewFolders(
      const BookmarkNode* bb_node,
      BookmarkEditorView::EditorNode* b_node,
      BookmarkEditorView::EditorNode* parent_b_node,
      const BookmarkNode** parent_bb_node);

  // Returns the current url the user has input.
  GURL GetInputURL() const;

  // Invoked when the url or title has possibly changed. Updates the background
  // of Textfields and ok button appropriately.
  void UserInputChanged();

  // Creates a new folder as a child of the selected node. If no node is
  // selected, the new folder is added as a child of the bookmark node. Starts
  // editing on the new gorup as well.
  void NewFolder();

  // Creates a new EditorNode as the last child of parent. The new node is
  // added to the model and returned. This does NOT start editing. This is used
  // internally by NewFolder and broken into a separate method for testing.
  EditorNode* AddNewFolder(EditorNode* parent);

  // If |editor_node| is expanded it's added to |expanded_nodes| and this is
  // recursively invoked for all the children.
  void UpdateExpandedNodes(EditorNode* editor_node,
                           BookmarkExpandedStateTracker::Nodes* expanded_nodes);

  ui::SimpleMenuModel* GetMenuModel();

  // Profile the entry is from.
  Profile* profile_;

  // Model driving the TreeView.
  scoped_ptr<EditorTreeModel> tree_model_;

  // Displays star folder.
  views::TreeView* tree_view_;

  // Used to create a new folder.
  scoped_ptr<views::LabelButton> new_folder_button_;

  // The label for the url text field.
  views::Label* url_label_;

  // The text field used for editing the URL.
  views::Textfield* url_tf_;

  // The label for the title text field.
  views::Label* title_label_;

  // The text field used for editing the title.
  views::Textfield* title_tf_;

  // Initial parent to select. Is only used if |details_.existing_node| is
  // NULL.
  const BookmarkNode* parent_;

  const EditDetails details_;

  // The context menu.
  scoped_ptr<ui::SimpleMenuModel> context_menu_model_;
  scoped_ptr<views::MenuRunner> context_menu_runner_;

  // Mode used to create nodes from.
  BookmarkModel* bb_model_;

  // If true, we're running the menu for the bookmark bar or other bookmarks
  // nodes.
  bool running_menu_for_root_;

  // Is the tree shown?
  bool show_tree_;

  // List of deleted bookmark folders.
  std::vector<int64> deletes_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkEditorView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_EDITOR_VIEW_H_
