#ifndef LINKEDLIST_HPP
#define LINKEDLIST_HPP

// simple doubly-linked list with sentinel head & tail

class Node {
 public:
  // node will point to index of the block/entry *in a set*
  uint64_t indexOfBlockOrEntry;
  Node* prev;
  Node* next;
  Node() : indexOfBlockOrEntry{0}, prev{nullptr}, next{nullptr} {}
  Node(uint64_t indexOfBlockOrEntry)
      : indexOfBlockOrEntry{indexOfBlockOrEntry},
        prev{nullptr},
        next{nullptr} {}
};

class LinkedList {
 public:
  Node* head;
  Node* tail;
  LinkedList() {
    head = new Node(0);
    tail = new Node(0);
    head->next = tail;
    tail->prev = head;
  }
  void pushBack(uint64_t indexOfBlockOrEntry) {
    Node* node = new Node(indexOfBlockOrEntry);
    node->next = tail;
    node->prev = tail->prev;
    tail->prev->next = node;
    tail->prev = node;
  }
  void pushFront(uint64_t indexOfBlockOrEntry) {
    Node* node = new Node(indexOfBlockOrEntry);
    node->next = head->next;
    node->prev = head;
    head->next->prev = node;
    head->next = node;
  }
  void pushFront(Node* node) {
    node->next = head->next;
    node->prev = head;
    head->next->prev = node;
    head->next = node;
  }
  void remove(Node* node) {
    node->next->prev = node->prev;
    node->prev->next = node->next;
  }
  // used to mark a specific node (which points to a block/entry)
  // as the MRU
  void moveToFront(Node* node) {
    remove(node);
    pushFront(node);
  }
  uint64_t front() const { return head->next->indexOfBlockOrEntry; }
  uint64_t back() const { return tail->prev->indexOfBlockOrEntry; }
  Node* frontNode() const { return head->next; }
  Node* backNode() const { return tail->prev; }
  ~LinkedList() {
    Node* tmp = head;
    Node* aux;
    while (tmp != nullptr) {
      aux = tmp;
      tmp = tmp->next;
      delete aux;
    }
  }
};

#endif