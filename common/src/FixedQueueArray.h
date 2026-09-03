/*
 *  FixedQueueArray.h
 *
 *  Library implementing a generic, fixed size circular queue in an array.
 *
 *  ---
 *
 *  Derived from QueueArray, Copyright (C) 2010  Efstathios Chatzikyriakidis (contact@efxa.org)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *  ---
 *
 *  Version 1.0
 *
 *    2021-07-19  Danny Levinson <danny@picolina.net>
 *
 *    - added ccopy, snprint
 *
 *    2018-04-22  Danny Levinson <danny@picolina.net>
 *    
 *      - converted to prespecified array; eliminated resizing
 *      - added peek
 *      - added user defined exit handling
 *      
 *    2014-02-03  Brian Fletcher  <brian.jf.fletcher@gmail.com>
 *
 *      - added enqueue(), dequeue() and front().
 *
 *    2010-09-29  Efstathios Chatzikyriakidis  <contact@efxa.org>
 *
 *      - added resize(): for growing, shrinking the array size.
 *
 *    2010-09-25  Efstathios Chatzikyriakidis  <contact@efxa.org>
 *
 *      - added exit(), blink(): error reporting and handling methods.
 *
 *    2010-09-24  Alexander Brevig  <alexanderbrevig@gmail.com>
 *
 *      - added setPrinter(): indirectly reference a Serial object.
 *
 *    2010-09-20  Efstathios Chatzikyriakidis  <contact@efxa.org>
 *
 *      - initial release of the library.
 *
 *  ---
 *
 *  For the latest version see: http://www.arduino.cc/
 */

// header defining the interface of the source.
#ifndef _FIXEDQUEUEARRAY_H
#define _FIXEDQUEUEARRAY_H

// include Arduino basic header.
#include <Arduino.h>

// the definition of the queue class.
template<typename T>
class FixedSizeQueueArray {
  public:
    // init the queue (constructor).
    FixedSizeQueueArray (int nelts);

    // clear the queue (destructor).
    ~FixedSizeQueueArray ();

    // copy from another array
    void ccopy(FixedSizeQueueArray<T> &src);

    // print contents comma separated using pattern
    void snprint(char *buf, int sz, char *patt);

    // add an item to the queue.
    void enqueue (const T i);
    
    // remove an item from the queue.
    T dequeue ();

    // push an item to the queue.
    void push (const T i);

    // pop an item from the queue.
    T pop ();

    // get the front of the queue.
    T front () const;

    // get an item from the queue.
    T peek () const;

    // get the nth item of the queue
    T get (int n) const;

    // check if the queue is empty.
    bool isEmpty () const;

    // get the number of items in the queue.
    int count () const;

    // check if the queue is full.
    bool isFull () const;

    // set the printer of the queue.
    void setPrinter (Print & p);
    void setExitHandler (void (*f)());
    
  private:

    // exit report method in case of error.
    void exit (const char * m) const;

    // led blinking method in case of error.
    void blink () const;

    // the pin number of the on-board led.
    static const int ledPin = D4;              // for Wemos D1 mini

    Print * printer; // the printer of the queue.
    void (*exitHandler)();

    T * contents;    // the array of the queue.

    int size;        // the size of the queue.
    int items;       // the number of items of the queue.

    int head;        // the head of the queue.
    int tail;        // the tail of the queue.
};

// init the queue (constructor).
template<typename T>
FixedSizeQueueArray<T>::FixedSizeQueueArray (int nelts) {
  size = 0;       // set the size of queue to zero.
  items = 0;      // set the number of items of queue to zero.

  head = 0;       // set the head of the queue to zero.
  tail = 0;       // set the tail of the queue to zero.

  printer = NULL; // set the printer of queue to point nowhere.
  exitHandler = NULL;

  // allocate enough memory for the array.
  contents = (T *) malloc (sizeof (T) * nelts);

  // if there is a memory allocation error.
  if (contents == NULL)
    exit ("QUEUE: insufficient memory to initialize queue.");

  // set the initial size of the queue.
  size = nelts;
}

// clear the queue (destructor).
template<typename T>
FixedSizeQueueArray<T>::~FixedSizeQueueArray () {
  free (contents); // deallocate the array of the queue.

  contents = NULL; // set queue's array pointer to nowhere.
  printer = NULL;  // set the printer of queue to point nowhere.

  size = 0;        // set the size of queue to zero.
  items = 0;       // set the number of items of queue to zero.

  head = 0;        // set the head of the queue to zero.
  tail = 0;        // set the tail of the queue to zero.
}

template<typename T>
void FixedSizeQueueArray<T>::ccopy(FixedSizeQueueArray<T> &src) {
//	for(int n=src.count() - 1 ;; n--) {
//		if (n <= 0) break;
//		enqueue(src.get(n));
//	}
	for(int n=0 ;; n++) {
		if (n >= src.count()) break;
		enqueue(src.get(n));
	}
}

template<typename T>
void FixedSizeQueueArray<T>::snprint(char *buf, int sz, char *patt) {
	char *p = buf;
	//snprintf(p, sz, "%d: ", count());
	//int l2 = strlen(p); p += l2; sz -= l2;
	for(int n=0 ;; n++) {
		if (n >= count()) break;
		if (n > 0) {
			snprintf(p, sz, ", ");
			int l = strlen(p);
			p += l;
			sz -= l;
			if (sz <= 0) break;
		}
		snprintf(p, sz, patt, get(n));
		int l = strlen(p);
		p += l;
		sz -= l;
		if (sz <= 0) break;
	}
}

// add an item to the queue.
template<typename T>
void FixedSizeQueueArray<T>::enqueue (const T i) {
  // check if the queue is full.
  if (isFull ())
    // discard the last item
    dequeue();

  // store the item to the array.
  contents[tail++] = i;
  
  // wrap-around index.
  if (tail == size) tail = 0;

  // increase the items.
  items++;
}

// push an item to the queue.
template<typename T>
void FixedSizeQueueArray<T>::push (const T i) {
  enqueue(i);
}

// remove an item from the queue.
template<typename T>
T FixedSizeQueueArray<T>::dequeue () {
  // check if the queue is empty.
  if (isEmpty ())
    exit ("QUEUE: can't pop item from queue: queue is empty.");

  // fetch the item from the array.
  T item = contents[head++];

  // decrease the items.
  items--;

  // wrap-around index.
  if (head == size) head = 0;

  // return the item from the array.
  return item;
}

// pop an item from the queue.
template<typename T>
T FixedSizeQueueArray<T>::pop () {
  return dequeue();
}

// get the front of the queue.
template<typename T>
T FixedSizeQueueArray<T>::front () const {
  // check if the queue is empty.
  if (isEmpty ())
    exit ("QUEUE: can't get the front item of queue: queue is empty.");
    
  // get the item from the array.
  return contents[head];
}

// get an item from the queue.
template<typename T>
T FixedSizeQueueArray<T>::peek () const {
  return front();
}

// get the nth item in the queue (0 is oldest, count()-1 is newest)
template<typename T>
T FixedSizeQueueArray<T>::get (int n) const {
  // check if the queue is empty.
  if (isEmpty ())
    exit ("QUEUE: can't get the first item of queue: queue is empty.");
  if (n >= items)
    exit ("QUEUE: can't get the nth item of queue: n >= count().");
  // get the item from the array.
  if (head + n < size)
    return contents[head + n];
  else
    return contents[n - (size - head)];
}

// check if the queue is empty.
template<typename T>
bool FixedSizeQueueArray<T>::isEmpty () const {
  return items == 0;
}

// check if the queue is full.
template<typename T>
bool FixedSizeQueueArray<T>::isFull () const {
  return items == size;
}

// get the number of items in the queue.
template<typename T>
int FixedSizeQueueArray<T>::count () const {
  return items;
}

// set the printer of the queue.
template<typename T>
void FixedSizeQueueArray<T>::setPrinter (Print & p) {
  printer = &p;
}
template<typename T>
void FixedSizeQueueArray<T>::setExitHandler (void (*f)()) {
  exitHandler = f;
}

// exit report method in case of error.
template<typename T>
void FixedSizeQueueArray<T>::exit (const char * m) const {
  // print the message if there is a printer.
  if (printer)
    printer->println (m);
  if (exitHandler) {
    (*exitHandler)();
  } else {
    // loop blinking until hardware reset.
    blink ();
  }
}

// led blinking method in case of error.
template<typename T>
void FixedSizeQueueArray<T>::blink () const {
  // set led pin as output.
  pinMode (ledPin, OUTPUT);

  // continue looping until hardware reset.
  while (true) {
    digitalWrite (ledPin, HIGH); // sets the LED on.
    delay (250);                 // pauses 1/4 of second.
    digitalWrite (ledPin, LOW);  // sets the LED off.
    delay (250);                 // pauses 1/4 of second.
  }

  // solution selected due to lack of exit() and assert().
}

#endif // _FIXEDQUEUEARRAY_H
