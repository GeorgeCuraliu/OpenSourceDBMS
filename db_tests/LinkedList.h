#pragma once

#include <functional>

#include "Column.h"

//t must have a next pointer member!
template <class t>
class LinkedList {
public:
	t* head, * tail;

	LinkedList() :head(nullptr), tail(nullptr) {}

	void AddNode(t* newNode) {
		if (!head) {
			head = newNode;
			tail = head;
			return;
		}
		else {
			tail->next = newNode;
			tail = tail->next;
		}
	}

	void IterateWithCallback(std::function<void(t*)> callBackFunc) {
		Column* temp = head;
		while (temp) {
			callBackFunc(temp);
			temp = temp->next;
		}
	}
	void DeleteNode(t* node) {
		if (node == head) {
			t* oldhead = head;
			head = head->next;
			delete oldhead;
		}
		t* temp = head;
		while (temp->next != node)
			temp = temp->next;
		t* toDelete = temp->next;
		temp->next = toDelete->next;
		delete toDelete;

	}
	void DeleteList() {
		t* temp = head;
		while (temp) {
			t* toDelete = temp;
			temp = temp->next;
			delete toDelete;
		}
		tail = nullptr;
	}

};