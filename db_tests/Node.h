#pragma once
template < typename t>
class Node{
public:

	Node<t> *parent, *left, *right;
	int balance : 3;

	t data;
	Node(t val, Node<t>* par);
};



template <typename t>
Node<t>::Node(t val, Node<t>* par) : parent(par), left(nullptr), right(nullptr), balance(0), data(val) {

}

