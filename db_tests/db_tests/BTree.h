#pragma once
#include "Node.h"

template <typename t>
class BTreeT{	
public:
	Node<t>* root;
	BTreeT();
	void addNode(t data);
	void updateBalance(Node<t>* insertedNode, Node<t>* insertedNodeParent);
	void rebalanceLeft(Node<t>* node);
	void rebalaceRight(Node<t>* node);
	void deleteNode(t data);
	void findNode(t data);
	void printInOrder(Node<t> *node);
};


template <typename t>
BTreeT<t>::BTreeT() : root(nullptr) {

}

template <typename t>
void BTreeT<t>::addNode(t data) {

	if (!root) {
		root = new Node<t>(data, nullptr);
	}
	else {
		bool finalPos = true;
		Node<t>* currentNode = root;
		Node<t>* newNode = new Node<t>(data, currentNode);
		while (finalPos) {
			if (data < currentNode->data) {
				!currentNode->left ? currentNode->left = newNode, finalPos = false : currentNode = currentNode->left;
			}
			else {
				!currentNode->right ? currentNode->right = newNode, finalPos = false : currentNode = currentNode->right;
			}
		}

		std::cout << data << " is located at "
			<< ((data < currentNode->data) ? " left " : "right ")
			<< "of " << currentNode->data << std::endl;

		updateBalance(newNode , currentNode);
	}
}

template<typename t>
void BTreeT<t>::updateBalance(Node<t>* insertedNode, Node<t>* insertedNodeParent){
	Node<t>* Q = insertedNode, * P = insertedNodeParent;
	Q == P->left ? P->balance-- : P->balance++;
	while (P != root && P->balance != 2 && P->balance != -2) {
		Q = P;
		P = P->parent;
		if (Q->balance == 0) return;
		Q == P->left ? P->balance-- : P->balance++;
		if (P->balance == -2) {
			rebalaceRight(P);
		}
		else if (P->balance == 2) {
			rebalanceLeft(P);
		}
	}
}

template<typename t>
void BTreeT<t>::rebalanceLeft(Node<t>* node){
	Node<t>* parent = node;

	if (node->right) {
		node = node->right;
		if (parent == root){
			root = parent->right;
			root->parent = nullptr;
		}
		parent->right = node->left;
		if (node->left) node->left->parent = parent;
		node->right->parent = parent;
		node->left = parent;
	}
	else {
		std::cout << "cant perform left rotation without a right node for parent " << node->data << " " << node->left << " " << node->right << " " << node->balance << std::endl;
	}
}

template<typename t>
void BTreeT<t>::rebalaceRight(Node<t>* node){
	Node<t>* parent = node;

	if (node->left) {
		node = node->left;
		if (parent == root) root = parent->left;
		parent->left = node->right;
		node->right = parent;
	}
	else {
		std::cout << "cant perform right rotation without a left node for parent " << node->data << " " << node->left << " " << node->right << " " << node->balance <<  std::endl;
	}
}


template<typename t>
void BTreeT<t>::printInOrder(Node<t>* node) {
	if (node != nullptr) {
		printInOrder(node->left);  // Visit left subtree
		std::cout << node->data << " ";  // Print current node
		printInOrder(node->right);  // Visit right subtree
	}
}