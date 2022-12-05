#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <pthread.h>

#define HAVE_STRUCT_TIMESPEC


static inline
void ec(int n)
{
    if (n) {
        //fprintf(stderr, "fallo thread: %s\n", strerror(n));
        abort();
    }
}


template <typename T, size_t capacidad = 256>
struct Concurrent_Queue
{
    T elements[capacidad];
    size_t begin;
    size_t size;
    pthread_mutex_t mutex;
    pthread_cond_t cond_full;
    pthread_cond_t con_finalizado;
    int flag;

    void finalizado()
    {
        ec(pthread_mutex_lock(&mutex));
        flag = 1;
        ec(pthread_mutex_unlock(&mutex));
        pthread_cond_signal(&con_finalizado);
    }

    void push(T elemento)
    {
        ec(pthread_mutex_lock(&mutex));
        while (size >= capacidad) {
            ec(pthread_cond_wait(&cond_full, &mutex));
        }

        elements[(begin + size) % capacidad] = elemento;
        size += 1;
        ec(pthread_mutex_unlock(&mutex));
        ec(pthread_cond_signal(&con_finalizado));
    }

    int pop(T* elemento)
    {
        ec(pthread_mutex_lock(&mutex));
        while (size == 0) {
            if (flag) {
                ec(pthread_mutex_unlock(&mutex));
                ec(pthread_cond_signal(&con_finalizado));
                return 0;
            }
            ec(pthread_cond_wait(&con_finalizado, &mutex));
        }

        if (elemento) *element = elements[begin];
        begin = (begin + 1) % capacidad;
        size -= 1;

        ec(pthread_mutex_unlock(&mutex));
        ec(pthread_cond_signal(&cond_full));

        return 1;
    }
};

const int element_c = 100;

void* consumer(void* arg)
{
    Concurrent_Queue<int>* queue = (Concurrent_Queue<int> *)arg;

    int x = 0;
    while (queue->pop(&x)) {
        fprintf(stderr, "%lu: %d\n", pthread_self(), x);
    }

    return NULL;
}

Concurrent_Queue<int> queue;
const size_t thread_c = 100;
pthread_t threads[thread_c];

int main()
{


    for (size_t i = 0; i < thread_c; ++i) {
        ec(pthread_create(&threads[i], NULL, consumer, &queue));
    }

    for (int i = 0; i < element_c; ++i) {
        queue.push(i);
    }
    queue.finalizado();

    for (size_t i = 0; i < thread_c; ++i) {
        ec(pthread_join(threads[i], NULL));
    }

    return 0;
}
