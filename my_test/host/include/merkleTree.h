/*************************************************************************
	> File Name: merkleTree.h
	> Author: 
	> Mail: 
	> Created Time: 2021年06月11日 星期五 20时31分23秒
 ************************************************************************/

#ifndef _MERKLETREE_H
#define _MERKLETREE_H
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <openssl/sha.h>
#include "sqlite3.h"
#define FILENAME "merkleTree.txt"
#define dbNameLen 50

//用来存储节点信息
struct dataHashNode{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    struct dataHashNode *left;
    struct dataHashNode *right;
    struct dataHashNode *parent;

    //页码信息
    unsigned int leftPgno;
    unsigned int rightPgno;

    //后续签名补充
}dataHashNode;

//用来存储当前构造的merkleTree信息
struct merkleTree{
    struct dataHashNode *root;
    char dbName[dbNameLen];
    unsigned int treeHeight;
    unsigned int nodeNum;
    //后续可能需要补充
}merkleTree;

//用来存取读取文件信息结构体
typedef struct storeInfo{
    //当前节点层数
    unsigned int level;
    //页码信息
    unsigned int leftPgno;
    unsigned int rightPgno;
    unsigned char hash[SHA256_DIGEST_LENGTH];
}storeInfo;

//创建一个新的树内部节点
struct dataHashNode* newNode();

//创建一个新的表示整个树的节点
struct merkleTree* newMerkleTree();

//创建一棵新树
/*
 *  data[]存放的是sqlite数据页面的哈希值
 *  p[]存放的时sqlite数据页面的页码
 *  nodeNums存放构建树时的初始化叶子节点
 *  dbName为要构建的数据库名字
*/
struct merkleTree* buildTree(unsigned char* data, unsigned int *p ,int nodeNums, const char* dbName);

//插入一个新节点
int insertNode(struct merkleTree* tree, struct dataHashNode* node);

//计算父节点的哈希值并更新其页码范围
void calcuHash(struct dataHashNode* node);

//输出当前树
void printTree(const struct dataHashNode* tree, int depth);

//将树节点信息存入文件
int writeToFile(const struct merkleTree* myTree);
void storeNode(struct dataHashNode* cur, unsigned int n);

//将节点信息从文件读出
int readToTree(struct merkleTree* myTree);
void rebuildNode(struct dataHashNode** pre, unsigned int* preN, const struct storeInfo* sInfo);

