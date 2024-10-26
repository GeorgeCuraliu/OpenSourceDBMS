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

	void IterateWithCallback(std::function<void(Column*)> callBackFunc) {
		Column* temp = head;
		while (temp) {
			callBackFunc(temp);
			temp = temp->next;
		}
	}

};