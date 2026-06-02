#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>


int main(int argc, char *argv[]) {

    /* --- FASE PREVIA: Limpiar el puerto de ejecuciones anteriores --- */
    printf("\n[FASE PREVIA] Liberando el puerto 4370...\n");
    pid_t pid_clean = fork();

    if (pid_clean < 0) {
        perror("[ERROR] fork() fase previa");
        exit(EXIT_FAILURE);
    }

    if (pid_clean == 0) {
    
        execlp("sh", "sh", "-c", "fuser -k 4370/tcp || true", NULL);
        
        /* Si llega aquí hubo un error crítico en exec, pero no detenemos el programa */
        exit(EXIT_SUCCESS); 
    }

    /* PADRE: Espera a que el puerto se libere antes de continuar con la Fase 1 */
    int status_clean;
    waitpid(pid_clean, &status_clean, 0);
    printf("[OK] Puerto limpio y listo.\n");

    /* --- Parametrizacion via argumentos --- */
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <sesion.txt> <archivo_render.qmd>\n", argv[0]);
        fprintf(stderr, "Ej:  %s sesion_linux.txt templates.qmd\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *archivo_sesion = argv[1];   /* sesion_linux.txt        */
    const char *archivo_render = argv[2];   /* templates.qmd           */

    printf("shellDoc Web - Orquestador\n");
    printf("] Sesion  : %s\n", archivo_sesion);
    printf(" Render  : %s\n", archivo_render);

    /* 
       FASE 1  Proceso hijo lanza el procesador hilos
       El binario './procesador_hilos' lee sesion_linux.txt
       y genera informacion.qmd usando 3 hilos + semaforos
 */
    printf("\n[FASE 1] Lanzando procesador de hilos...\n");

    pid_t pid1 = fork();

    if (pid1 < 0) {
        perror("[ERROR] fork() fase 1");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        /* HIJO: ejecuta el procesador de hilos */
        /* Nota: procesador_hilos lee sesion_linux.txt desde el CWD */
        execlp("./procesador_hilos", "./procesador_hilos", NULL);
        perror("[ERROR] execlp procesador_hilos");
        exit(EXIT_FAILURE);
    }

    /* PADRE: espera que los hilos terminen */
    int status1;
    waitpid(pid1, &status1, 0);

    if (WIFEXITED(status1) && WEXITSTATUS(status1) == 0) {
        printf("[OK] Hilos completados. informacion.qmd generado.\n");
    } else {
        fprintf(stderr, "[ERROR] procesador_hilos fallo (codigo %d)\n",
                WEXITSTATUS(status1));
        exit(EXIT_FAILURE);
    }

    /* 
       FASE 2  Proceso hijo renderiza con Quarto
       Renderiza templates.qmd (que incluye informacion.qmd)
      */
    printf("\n[FASE 2] Renderizando pagina web con Quarto...\n");

    pid_t pid2 = fork();

    if (pid2 < 0) {
        perror("[ERROR] fork() fase 2");
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        /* HIJO: ejecuta quarto render */
        execlp("quarto", "quarto", "render", archivo_render, NULL);
        perror("[ERROR] execlp quarto");
        exit(EXIT_FAILURE);
    }

    int status2;
    waitpid(pid2, &status2, 0);

    if (WIFEXITED(status2) && WEXITSTATUS(status2) == 0) {
        printf("[OK] Quarto renderizo exitosamente.\n");
    } else {
        fprintf(stderr, "[ERROR] quarto render fallo (codigo %d)\n", WEXITSTATUS(status2));
        exit(EXIT_FAILURE);
    }

    pid_t pid3 = fork();

    if (pid3 < 0) {
        perror("[ERROR] fork() fase 3");
        exit(EXIT_FAILURE);
    }

    if (pid3 == 0) {
        /* HIJO: ejecuta quarto preview en un puerto específico */
        execlp("quarto", "quarto", "preview", archivo_render, "--port", "4370", NULL);
        
        /* Si llega a esta línea, el comando falló */
        perror("[ERROR] execlp quarto");
        exit(EXIT_FAILURE);
    }



    pid_t pid4 = fork();

    if (pid4 < 0) {
        perror("[ERROR] fork() fase 4");
        exit(EXIT_FAILURE);
    }

    if (pid4 == 0) {
        /* HIJO: Sube los cambios a github automaticamente */
        execlp("sh", "sh", "-c", "git add . ; git commit -m 'Guardado automático' ; git push origin main", NULL);
        
        /* Si llega a esta línea, el comando falló */
        perror("[ERROR] execlp git automatizacion");
        exit(EXIT_FAILURE);
    }
 
    int status3;
    waitpid(pid4, &status3, 0);

    if (WIFEXITED(status3) && WEXITSTATUS(status3) == 0) {
        printf("[OK] ¡Código y documentación subidos a GitHub con éxito!\n");
        printf("-> GitHub Actions está publicando tu página en la nube.\n");
        printf("\n=== ShellDoc completado con éxito ===\n");
    } else {
        fprintf(stderr, "[WARN] No se pudo subir a GitHub. Revisa el estado de Git.\n");
    }

    return EXIT_SUCCESS;
}
