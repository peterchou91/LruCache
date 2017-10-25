#include <iostream>
#include <atomic>
#include <vector>
#include <chrono>
using namespace std;
using namespace chrono;
using vector=std::vector<int>;
typedef struct Node{
    int data;
    Node(int n):data(n){
        std::cout << "construct Node" << std::endl;
    }
    Node(Node& node){
        std::cout << "copy construct node" << std::endl;
    }
    Node(){}
    Node(Node&& node){}
} Node;

template <typename k>
class Person{
public:
    k v;
    Person(int n){

    }
    void set(k key){
        this->v = key;
        std::cout << v << endl;
    }
    void get(){
        std::cout << v << endl;
    }
};

int main() {
    /*
    std::atomic_long l = {0};

    auto start = system_clock::now();
    auto time = (duration_cast<milliseconds>(start.time_since_epoch())).count();
    l = 9;
    std::atomic<Node*> n1;
    n1.store(new Node());
    n1.load()->data = 9;
    Node* n2Ptr = new Node();
    std::cout << "n2:" << n2Ptr << std::endl;
    n2Ptr->data = 8;
    n1.exchange(n2Ptr);
    //n1.compare_exchange_weak(n1.load(),n2Ptr,std::memory_order_acquire,std::memory_order_acquire);
    std::cout << "n2:" << n2Ptr << std::endl;
    std::cout << n1.load()->data << std::endl;
    std::cout << n2Ptr->data << std::endl;
    Person p = 10;
    Node node1(1);
    Node cpNode = node1;
    Node mvNode = std::move(node1);
    std::cout << "mvNode data:" << mvNode.data << " addr:" << &mvNode << std::endl;
    std::cout << "oriNode data" << node1.data << "addr:" << &node1 << std::endl;
    std::cout << "cpNode data" << cpNode.data << "addr:" << &cpNode << std::endl;
     */
    Person<int> p(1);
    p.set(10);
    p.get();
    return 0;
}