#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>

//compile with -lpam -lpam_misc

struct pam_conv my_conv =
        {
                misc_conv,
                NULL,
        };

int login_into_user(char *username)
{
    pam_handle_t *pam;
    int ret;

    ret = pam_start("my_ssh", username, &my_conv, &pam);

    if (ret != PAM_SUCCESS)
    {
        printf("Failed pam_start\n");
        return -1;
    }

    ret = pam_authenticate(pam, 0);

    if (ret != PAM_SUCCESS)
    {
        printf("Incorrect password!\n");
        return -1;
    }

    ret = pam_acct_mgmt(pam, 0);

    if (ret != PAM_SUCCESS)
    {
        printf("User account expired!\n");
        return -1;
    }

    if (pam_end(pam, ret) != PAM_SUCCESS)
    {
        printf("Unable to pam_end()\n");
        return -1;
    }

    printf("Login succesfull\n");
    return 0;
}

int main(int argc, char **argv)
{
    struct passwd *info;

    if (argc != 2)
    {
        printf("Correct usage example:  ./a.out <username>\n");
        return -1;
    }

    info = getpwnam(argv[1]);

    if (!info)
    {
        perror("getpwnam");
        return -1;
    }

    printf("name: %s, uid: %d\n", info->pw_name, info->pw_uid);

    if (login_into_user(argv[1]))
    {
        printf("Unsuccessful authentification for user %s\n", argv[1]);
        return -1;
    }

    return 0;
}