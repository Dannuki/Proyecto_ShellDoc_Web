#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_TEXT 500000000

char texto_original[MAX_TEXT];
char texto_limpio[MAX_TEXT];
char texto_normalizado[MAX_TEXT];

sem_t sem1;
sem_t sem2;

/* HILO 1 */
void *limpieza(void *arg)
{
    int j = 0;

    for(int i=0; texto_original[i] != '\0'; i++)
    {
        unsigned char c = texto_original[i];

        if(c >= 32 || c == '\n' || c == '\t')
        {
            texto_limpio[j++] = c;
        }
    }

    texto_limpio[j] = '\0';

    printf("Hilo 1 terminado\n");

    sem_post(&sem1);

    pthread_exit(NULL);
}

/* HILO 2 */
void *normalizacion(void *arg)
{
    sem_wait(&sem1);

    int j = 0;
    int salto = 0;

    for(int i=0; texto_limpio[i] != '\0'; i++)
    {
        if(texto_limpio[i] == '\n')
        {
            if(salto)
                continue;

            salto = 1;
        }
        else
        {
            salto = 0;
        }

        texto_normalizado[j++] = texto_limpio[i];
    }

    texto_normalizado[j] = '\0';

    printf("Hilo 2 terminado\n");

    sem_post(&sem2);

    pthread_exit(NULL);
}

/* HILO 3 */
void *markdown(void *arg)
{
    sem_wait(&sem2);

    FILE *out = fopen("web/informacion.qmd","w");

    fprintf(out,
        "---\n"
        "title: \"ShellDoc Web\"\n"
        "format: html\n"
        "---\n\n"
    );

    fprintf(out,"# Sesion Linux\n\n");

    fprintf(out,"```bash\n");

    fprintf(out,"%s",texto_normalizado);

    fprintf(out,"\n```\n");

    fclose(out);

    printf("Archivo informacion.qmd generado\n");

    pthread_exit(NULL);
}

int main()
{
    FILE *f = fopen("sesion_linux.txt","r");

    if(f == NULL)
    {
        printf("No se pudo abrir sesion_linux.txt\n");
        return 1;
    }

    fread(texto_original,1,MAX_TEXT,f);

    fclose(f);

    pthread_t t1,t2,t3;

    sem_init(&sem1,0,0);
    sem_init(&sem2,0,0);

    pthread_create(&t1,NULL,limpieza,NULL);
    pthread_create(&t2,NULL,normalizacion,NULL);
    pthread_create(&t3,NULL,markdown,NULL);

    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    pthread_join(t3,NULL);

    sem_destroy(&sem1);
    sem_destroy(&sem2);
    printf("Proceso completado\n");

    return 0;
}
