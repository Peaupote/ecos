#include <kernel/tty.h>

#include <libc/string.h>
#include <kernel/kutil.h>

#include <kernel/memory/kmem.h>
#include <kernel/proc.h>
#include <kernel/tests.h>
#include <fs/ext2.h>

static void ls (char** tokpt) {
    char* arg1 = strtok_rnull(NULL, " ", tokpt);
    if (!arg1) return;

	vfile_t *vf = vfs_load(arg1, 0);
    if (!vf) {
        kprintf("%s don't exist\n", arg1);
        return;
    }

	chann_adt_t cdt;
	vfs_opench(vf, &cdt);

    char dbuf[512];

	struct device *dev = devices + vf->vf_stat.st_dev;
	struct fs *fs = fst + dev->dev_fs;
	struct stat st;

	kprintf("size %d\n", vf->vf_stat.st_size);
	kprintf("INO    TYPE    SIZE    NLINK        NAME\n");

	int rc;
	while((rc = vfs_getdents(vf, (struct dirent*)dbuf, 512, &cdt)) > 0) {
		struct dirent* de = (struct dirent*)dbuf;
		if (rc < de->d_rec_len) {
			klogf(Log_error, "ls", 
					"pas assez de place pour stocker le nom");
			break;
		}
		for (int i = 0; i < rc;
				i += de->d_rec_len, de = (struct dirent*)(dbuf + i)) {
			fs->fs_stat(de->d_ino, &st, &dev->dev_info);
			kprintf("%d    %x    %d    %d        %s\n",
					de->d_ino, st.st_mode, st.st_size, st.st_nlink,
					de->d_name);
		}
	}

    vfs_close(vf);
}

static void inspect_vfiles() {
    kprintf("FILE    INO    DEV    CNT\n");
    for (size_t i = 0; i < NFILE; i++) {
        vfile_t *vf = state.st_files + i;
        if (vf->vf_cnt > 0)
            kprintf("%d    %d    %d    %d\n",
                    i, vf->vf_stat.st_ino, vf->vf_stat.st_dev, vf->vf_cnt);
    }
}

static void inspect_channs() {
    kprintf("CID    VFILE    MODE    ACC\n");
    for (size_t i = 0; i < NCHAN; i++) {
        chann_t *c = state.st_chann + i;
        if (c->chann_acc > 0) {
            kprintf("%d    %d    %d    %d\n",
                    i, (c->chann_vfile - state.st_files) / sizeof(vfile_t),
                    c->chann_mode, c->chann_acc);
        }
    }
}

static uint_ptr read_ptr(const char str[]) {
	uint_ptr rt = 0;
    if (str[0] == 'k') {
		sscanf(str + 1, "%p", &rt);
		rt += paging_add_lvl(pgg_pml4, PML4_KERNEL_VIRT_ADDR);
	} else
		sscanf(str, "%p", &rt);
    return rt;
}

/*
 * log [-options] [hd]
 * hd: filtre d'en-tête, '*' pour aucun filtre
 * options:
 *   -0 	aucun
 *   -e 	erreurs
 *   -w 	warnings
 *   -i 	infos
 *   -v		verbose
 *   -vv	very verbose
 * */
static void change_log(char** tokpt) {
	char* arg;
	while ((arg = strtok_rnull(NULL, " ", tokpt))) {
		if (arg[0] == '-') {
			switch(arg[1]) {
				case 'e':
					klog_level = Log_error;
					break;
				case 'w':
					klog_level = Log_warn;
					break;
				case 'i':
					klog_level = Log_info;
					break;
				case 'v':
					klog_level = arg[2] == 'v' ? Log_vverb : Log_verb;
					break;
				default:
					klog_level = Log_none;
			}
		} else if (!strcmp(arg, "*"))
			*klog_filtr = '\0';
		else if (strlen(arg) < 256)
			strcpy(klog_filtr, arg);
	}
}

void tty_built_in_exec(tty_seq_t* sq, char* cmd) {
    char* tokpt;
    char* cmd_name = strtok_rnull(cmd, " ", &tokpt);
    if (!cmd_name) return;

    if (!strcmp(cmd_name, "memat")) {
        char* arg1 = strtok_rnull(NULL, " ", &tokpt);
        if (!arg1) return;
        uint_ptr ptr = read_ptr(arg1);
		tty_seq_printf(sq, "%hhx", *(char*)ptr);

    } else if (!strcmp(cmd_name, "pg2")) {
        char* arg1 = strtok_rnull(NULL, " ", &tokpt);
        if (!arg1) return;
        uint_ptr ptr = read_ptr(arg1);
        kmem_print_paging(ptr);

    } else if (!strcmp(cmd_name, "kprint"))
        tty_do_kprint = !tty_do_kprint;

    else if (!strcmp(cmd_name, "a"))
        use_azerty = 1;

    else if (!strcmp(cmd_name, "q"))
        use_azerty = 0;

    else if (!strcmp(cmd_name, "test")) {
        char* arg1 = strtok_rnull(NULL, " ", &tokpt);
        if (!arg1) return;
        if (!strcmp(arg1, "statut"))      test_print_statut();
        else if(!strcmp(arg1, "kheap"))   test_kheap();

    } else if (!strcmp(cmd_name, "ps"))
        proc_ps();

    else if (!strcmp(cmd_name, "unblock")) {
        char* arg1 = strtok_rnull(NULL, " ", &tokpt);
        if (!arg1) return;
        char* arg2 = strtok_rnull(NULL, " ", &tokpt);
        if (!arg2) return;
        int   ipid, val = 0;
        sscanf(arg1, "%d", &ipid);
        pid_t pid = ipid;
        sscanf(arg2, "%i", &val);
        state.st_proc[pid].p_reg.r.rdi.i = val;
        proc_unblock_1(pid, state.st_proc + pid);

    } else if (!strcmp(cmd_name, "kill")) {
        char* arg1 = strtok_rnull(NULL, " ", &tokpt);
        if (!arg1) return;
        char* arg2 = strtok_rnull(NULL, " ", &tokpt);
        if (!arg2) return;
        int   ipid, signum = 0;
        sscanf(arg1, "%d", &ipid);
        pid_t pid = ipid;
        sscanf(arg2, "%i", &signum);

        pid_t cproc = state.st_curr_pid;
        bool spr = pid == state.st_curr_pid;
        bool afc =  1  == send_sig_to_proc(pid, signum - 1);
        if (spr && afc)
            klogf(Log_error, "warning", "processus courant");
        if (cproc != state.st_curr_pid)
            switch_proc(cproc);
    }
	
	else if (!strcmp(cmd_name, "ls"))     ls(&tokpt);
    else if (!strcmp(cmd_name, "vfiles")) inspect_vfiles();
    else if (!strcmp(cmd_name, "channs")) inspect_channs();
	else if (!strcmp(cmd_name, "log"))    change_log(&tokpt);

    return;
}
