#include <cassert>
#include <iostream>
#include <string>

class linListData {
  std::string s;

public:
  linListData(std::string _s) : s(_s) {}
  void dump(std::ostream &os) { os << s; }
  friend class linListElement;
  friend class linList;
};

class linList;
class linListElement {
    linListData *data;
    linListElement *next;

   public:
    linListElement(linListData *_data) : data(_data), next(nullptr) {}
    ~linListElement() { delete data; }
    void dump(std::ostream &os) { data->dump(os); }
    void dumptoend(std::ostream &os) {
        linListElement *p = this;
        while (p) {
            std::cerr << "------------\n";
            std::cerr << "data: " << p->data->s << "\n";
            std::cerr << "next: " << p->next << "\n";
            std::cerr << "------------\n";
            p = p->next;
        }
        std::cerr << "End of list\n";
    }

    friend class linList;
};

class linList {
    linListElement head;

    linListElement *reverse_worker(linListElement *p,
                                   linListElement *new_head) {
        linListElement *new_element = nullptr;
        if (p->next) {
            linListElement *predecessor = reverse_worker(p->next, new_head);
            new_element = new linListElement(new linListData(p->data->s));
            predecessor->next = new_element;
            return new_element;
        } else {
            new_element = new linListElement(new linListData(p->data->s));
            new_head->next = new_element;
            return new_element;
        }
    }

   public:
    linList() : head(nullptr) {}
    ~linList() {
        linListElement *p = head.next;
        while (p) {
            linListElement *t = p;
            p = p->next;
            delete t;
        }
    }

    void addElement(std::string s) {
        linListElement *p = new linListElement(new linListData(s));
        p->next = head.next;
        head.next = p;
  }

  linListElement *first() { return head.next; }

  bool empty() { return first() == nullptr; }

  void dump(std::ostream &os) {
      linListElement *p = first();
      while (p) {
          p->dump(os);
          p = p->next;
      }
      os << "\n";
  }

  linList reverse() {
      linList rev;
      reverse_worker(first(), &rev.head);
      return rev;
  }
};

int main() {
  linList l;

  std::cout << "list is empty: " << l.empty() << "\n";
  l.addElement("This");
  l.addElement(" is");
  l.addElement(" a");
  l.addElement(" Test");
  l.addElement("!\n");
  std::cout << "list is empty: " << l.empty() << "\n";

  l.dump(std::cerr);

  linList reversed_list = l.reverse();

  std::cout << "Reversed:\n";
  reversed_list.dump(std::cerr);

  return 0;
}
