/*******************************************************************************
 * dirtree.c		Author: Ian Nobile
 *
 * C program that takes the absolute path of a given directory name as a
 * command line argument and recursively builds a tree data structure,
 * traversing the disk structure in a depth first search order. After building
 * the tree, the program prints the level, order, and absolute path of all
 * files and sub-directories level by level using a queue data structure. Tree,
 * linked list and queue data structures are all present as is leak-free
 * dynamic memory allocation, the use of readdir() to process the content of
 * directories and the use of stat() to differentiate directories from files.
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>


 //-----------------------------------------------------------------------------
 //	Structs
 //-----------------------------------------------------------------------------

struct LList {
	struct TreeNode *head; //	first child
	struct TreeNode *tail; //	last child
};

struct TreeNode {
	char *fileName;
	int level;
	struct TreeNode *nextSibling;
	struct LList *children;
};

struct Queue {
	struct QNode *lastIn;
	struct QNode *firstOut;
};

struct QNode {
	struct TreeNode *dataSource;
	struct QNode *queuePrev;
};


//-----------------------------------------------------------------------------
//	Function Prototypes
//-----------------------------------------------------------------------------

struct LList *createLList();

struct TreeNode *createTreeNode(char *name, int lvl);

void chopTree(struct TreeNode *root);

void appendChild(struct TreeNode *parent, struct TreeNode *child);

struct QNode *createQNode(struct TreeNode *node);

struct Queue *createQueue();

void enQueue(struct Queue *queue, struct TreeNode *tNode);

struct QNode *deQueue(struct Queue *queue);

struct Queue *createPrintQueue(struct TreeNode *root);

void printPrintQueue(struct Queue *printQueue);

void treePopulator(struct TreeNode *parentNode);


//-----------------------------------------------------------------------------
//	The Main Function
//-----------------------------------------------------------------------------

int main(int argc, char *argv[]) {

	char startPath[256];

	if (argc > 1) {
		strcpy(startPath, argv[argc - 1]);
	} else {
		strcpy(startPath, "/home/hal9k/home/naltipar/Downloads/final-src");
	}

	//	create root of tree with the starting path and populate
	struct TreeNode *root = createTreeNode(startPath, 1);
	treePopulator(root);

	//	traverse tree level by level and create print queue and print
	struct Queue *rootQueue = createPrintQueue(root);
	printPrintQueue(rootQueue);

	//	deallocate memory of each entry within tree and nullify
	chopTree(root);
	rootQueue = NULL;

	//	stop Valgrind's "FILE DESCRIPTORS open at exit" error:
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);

	//	obtain user confirmation before exiting
	getchar();
	return 0;

}//	end main


//------------------------------------------------------------------------------
//	Function Definitions
//------------------------------------------------------------------------------

//	linked list creator
struct LList *createLList() {
	struct LList *children = malloc(sizeof(struct LList));
	if (children == NULL) {
		printf("Sorry, but memory was found to be unallocatable for the llist.");
		exit(-1);
	}
	children->head = NULL;
	children->tail = NULL;

	return children;
}

//	create nodes in tree for each file/directory
struct TreeNode *createTreeNode(char *name, int lvl) {
	struct TreeNode *newNode = malloc(sizeof(struct TreeNode));
	if (newNode == NULL) {
		printf("Sorry, but memory was found to be unallocatable for the node.");
		exit(-1);
	}
	newNode->fileName = strdup(name);
	newNode->level = lvl;
	newNode->nextSibling = NULL;
	newNode->children = createLList();

	return newNode;
}

//	deallocate memory of each entry within tree and nullify pointers
void chopTree(struct TreeNode *root) {
	if (root != NULL) {
		//	depth first recursion
		if (root->children != NULL) {
			if (root->children->head != NULL) {
				chopTree(root->children->head);
			}
		}
		//	breadth recursion
		if (root->nextSibling != NULL) {
			chopTree(root->nextSibling);
		}

		//	memory deallocation and pointer nullification
		free(root->children);
		root->children = NULL;
		root->nextSibling = NULL;
		free(root->fileName);
		root->fileName = NULL;
		free(root);
		root = NULL;
	}
}

//	graft the newly created node into the tree
void appendChild(struct TreeNode *parent, struct TreeNode *child) {
	//	if a new list
	if (parent->children->head == NULL && parent->children->tail == NULL) {
		parent->children->head = child;
		parent->children->tail = child;
	} else {
		//	if an already filled list
		parent->children->tail->nextSibling = child;
		parent->children->tail = child;
	}
}

//	create a QNode from a TreeNode
struct QNode *createQNode(struct TreeNode *node) {
	struct QNode *newQNode = malloc(sizeof(struct QNode));	//	free malloc!
	if (newQNode == NULL) {
		printf("Sorry, but memory was found to be unallocatable for the queue node to be created.");
		exit(-1);
	}
	//	wrap the existing tree node inside of the new queue node
	newQNode->dataSource = node;
	//	and b/c the tree node *nextSibling points in the wrong direction:
	newQNode->queuePrev = NULL;

	return newQNode;
}

//	queue creator 
struct Queue *createQueue() {
	struct Queue *newQueue = malloc(sizeof(struct Queue));//	free malloc!
	if (newQueue == NULL) {
		printf("Sorry, but memory was found to be unallocatable for the queue to be created.");
		exit(-1);
	}
	newQueue->lastIn = NULL;
	newQueue->firstOut = NULL;
	return newQueue;
}

//	add queue node to queue
void enQueue(struct Queue *queue, struct TreeNode *tNode) {
	struct QNode *qNode = createQNode(tNode);
	//for first in queue
	if (queue->lastIn == NULL) {
		queue->firstOut = qNode;
		queue->lastIn = qNode;
		return;
	}
	queue->lastIn->queuePrev = qNode;
	queue->lastIn = qNode;
	return;
}

//	remove node from front of queue and return it
struct QNode *deQueue(struct Queue *queue) {
	if (queue->firstOut == NULL) {
		//	for an empty queue
		struct QNode *endDoWhile = malloc(sizeof(struct QNode));
		endDoWhile->dataSource = NULL;
		endDoWhile->queuePrev = NULL;
		return endDoWhile;
	} else if (queue->lastIn == queue->firstOut) {
		//	for the final item in a queue
		struct QNode *firstOut = queue->firstOut;
		queue->firstOut = NULL;
		queue->lastIn = NULL;
		return firstOut;
	} else {
		//	normal use
		struct QNode *firstOut;
		firstOut = queue->firstOut;
		queue->firstOut = queue->firstOut->queuePrev;
		return firstOut;
	}
}

//	traverse the built tree in breadth-first order and enqueue each node
struct Queue *createPrintQueue(struct TreeNode *root) {
	struct Queue *newPrintQueue = createQueue();
	struct Queue *toDoQueue = createQueue();
	struct TreeNode *treeClimber = root;
	struct TreeNode *branchClimber;

	//	treeClimber stands at parent node
	while (treeClimber != NULL) {
		enQueue(newPrintQueue, treeClimber);
		branchClimber = treeClimber->children->head;
		//	whilst branchClimber enqueues the children
		while (branchClimber != NULL) {
			enQueue(toDoQueue, branchClimber);
			branchClimber = branchClimber->nextSibling;
		}
		//	handler allows for deallocation
		struct QNode *QNHandler = deQueue(toDoQueue);
		treeClimber = QNHandler->dataSource;
		free(QNHandler);
		QNHandler = NULL;
	}
	//more memory deallocation
	free(toDoQueue->firstOut);
	toDoQueue->firstOut = NULL;
	free(toDoQueue->lastIn);
	toDoQueue->lastIn = NULL;
	free(toDoQueue);
	toDoQueue = NULL;

	return newPrintQueue;
}

//	print, deallocate and nullify the created print queue
void printPrintQueue(struct Queue *printQueue) {
	struct QNode *printer;
	int order = 0;
	int prevLevel = 0;

	do {
		printer = deQueue(printQueue);
		if (printer->dataSource->level != prevLevel) {
			order = 0;
		}
		order++;
		printf("%d:%d:%s\n", printer->dataSource->level, order, printer->dataSource->fileName);
		prevLevel = printer->dataSource->level;

		//	deallocation/nullification
		printer->queuePrev = NULL;
		free(printer);
		printer = NULL;
	} while (printQueue->lastIn != NULL);

	free(printQueue);
	printQueue = NULL;
}

//	crawl through root directory, create nodes and string them together
void treePopulator(struct TreeNode *parentNode) {
	DIR *directory;
	struct dirent *entry;

	directory = opendir(parentNode->fileName);
	if (directory == NULL) {
		printf("Something's gone awry!");
	}

	while ((entry = readdir(directory)) != 0) {
		//  skip over current . and root .. directories
		if (strcmp(entry->d_name, ".") == 0
			|| strcmp(entry->d_name, "..") == 0
			//	skip over hidden files (an unexpected requirement!)
			|| strncmp(entry->d_name, ".", 1) == 0) {
			continue;
		}

		//	note unique path of current file
		char childPath[256];
		strcpy(childPath, parentNode->fileName);
		strcat(childPath, "/");
		strcat(childPath, entry->d_name);

		//	fills stat variable with analysis of current entry
		struct stat status;
		stat(childPath, &status);

		//creates node from path and notes new level
		struct TreeNode *childNode = createTreeNode(childPath, parentNode->level + 1);

		//grafts node into parent->children 
		appendChild(parentNode, childNode);

		//	recurse if current entry is a folder
		if (S_ISDIR(status.st_mode)) {
			treePopulator(parentNode->children->tail);
		}
	}

	closedir(directory);

}

