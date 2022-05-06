#ifndef INCLUDES_PAM_FUNCS
#define INCLUDES_PAM_FUNCS

#include <pwd.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>

#define INCORRECT_PASSWORD 1

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
        return INCORRECT_PASSWORD;
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

#endif //INCLUDES_PAM_FUNCS