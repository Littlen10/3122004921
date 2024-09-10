#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define BITS 64
#define HASH_SIZE 256

// 定义哈希表节点
typedef struct hash_node
{
    uint64_t hash;          // 哈希值
    int count;              // 重复次数
    struct hash_node *next; // 下一个节点
} hash_node_t;

// 定义哈希表结构
typedef struct hash_table
{
    hash_node_t **nodes; // 节点数组
    int size;            // 大小
} hash_table_t;

// Simhash算法，输入为文本字符串和哈希表
uint64_t simhash(const char *text, hash_table_t *ht)
{
    uint64_t hash[BITS] = {0};
    char *token;
    char *saveptr;
    token = strtok_r((char *)text, " \t\n\r", &saveptr);
    while (token != NULL)
    {
        uint64_t h = 0;
        char *p = token;
        while (*p)
        {
            h = *(p++) + (h << 6) + (h << 16) - h;
        }
        for (int i = 0; i < BITS; i++)
        {
            if ((h >> i) & 1)
            {
                hash[i]++;
            }
            else
            {
                hash[i]--;
            }
        }
        token = strtok_r(NULL, " \t\n\r", &saveptr);
    }
    uint64_t simhash = 0;
    for (int i = 0; i < BITS; i++)
    {
        if (hash[i] > 0)
        {
            simhash |= (1ULL << i);
        }
    }
    // 将Simhash值加入哈希表
    hash_node_t *node = ht->nodes[simhash % ht->size];
    while (node != NULL && node->hash != simhash)
    {
        node = node->next;
    }
    if (node == NULL)
    {
        node = (hash_node_t *)malloc(sizeof(hash_node_t));
        node->hash = simhash;
        node->count = 1;
        node->next = ht->nodes[simhash % ht->size];
        ht->nodes[simhash % ht->size] = node;
    }
    else
    {
        node->count++;
    }
    return simhash;
}

// 加载文件，返回文件内容字符串
char *load_file(const char *filename)
{
    FILE *fp;
    if ((fp = fopen(filename, "rb")) == NULL)
    {
        printf("Failed to open file %s\n", filename);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buffer = (char *)malloc(size + 1);
    if (buffer == NULL)
    {
        printf("Failed to allocate memory for file %s\n", filename);
        return NULL;
    }
    fread(buffer, size, 1, fp);
    fclose(fp);
    buffer[size] = '\0';
    return buffer;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Usage: simhash <original_file> <plagiarized_file> <output_file>\n");
        return 0;
    }
    // 加载原文件和抄袭文件
    char *orig_text = load_file(argv[1]);
    char *plag_text = load_file(argv[2]);
    if (orig_text == NULL || plag_text == NULL)
    {
        return 0;
    }
    // 初始化哈希表
    hash_table_t ht;
    ht.nodes = (hash_node_t **)calloc(HASH_SIZE, sizeof(hash_node_t *));
    ht.size = HASH_SIZE;
    // 计算Simhash值
    uint64_t orig_simhash = simhash(orig_text, &ht);
    uint64_t plag_simhash = simhash(plag_text, &ht);
    // 计算汉明距离，即Simhash值的不同位数
    uint64_t distance = 0;
    uint64_t diff = orig_simhash ^ plag_simhash;
    while (diff > 0)
    {
        if (diff & 1)
        {
            distance++;
        }
        diff >>= 1;
    }
    // 将相似度写入输出文件
    FILE *fp = fopen(argv[3], "wb");
    if (fp == NULL)
    {
        printf("Failed to open file %s for output\n", argv[3]);
        return 0;
    }
    fprintf(fp, "%.2f%%\n", (1 - distance / (double)BITS) * 100);
    fclose(fp);
    return 0;
}