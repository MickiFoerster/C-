#include <iostream>
#include <memory>
#include <string>

class linListData {
  std::string s;

public:
  linListData(std::string _s) : s(_s) {}
  void dump(std::ostream &os) { os << s; }
};

class linList;
class linListElement {
  std::shared_ptr<linListData> data;
  std::shared_ptr<linListElement> next;

public:
  linListElement(std::shared_ptr<linListData> _data)
      : data(_data), next(nullptr) {}
  void dump(std::ostream &os) { data->dump(os); }
  friend class linList;
};

class linList {
  linListElement head;

public:
  linList() : head(nullptr) {}
  void addElement(std::string s) {
    std::shared_ptr<linListData> d = std::make_shared<linListData>(s);
    std::shared_ptr<linListElement> p = std::make_shared<linListElement>(d);
    p->next = head.next;
    head.next = p;
  }

  void dump(std::ostream &os) {
    std::shared_ptr<linListElement> p = head.next;
    while (p) {
      p->dump(os);
      p = p->next;
    }
  }
};

int main() {
  linList l;
  l.addElement("\n");
  l.addElement(" Test");
  l.addElement(" a");
  l.addElement(" is");
  l.addElement("This");

  l.dump(std::cerr);
  return 0;
}
