/* Self-contained test: build a fake iPod Classic tree, add tracks, write the signed iTunesDB,
 * re-parse it and check the contents. Needs no hardware. */
#include <gpod/itdb.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); return 1; } } while (0)

int main(void) {
    GError *err = NULL;
    gchar *root = g_dir_make_tmp("gpodtestXXXXXX", &err);
    CHECK(root);
    gchar *devdir = g_build_filename(root, "iPod_Control", "Device", NULL);
    CHECK(g_mkdir_with_parents(devdir, 0755) == 0);
    gchar *sysinfo = g_build_filename(devdir, "SysInfo", NULL);
    CHECK(g_file_set_contents(sysinfo,
        "ModelNumStr: xMB565\nFirewireGuid: 000A270012345678\npszSerialNumber: TEST123\n", -1, &err));
    CHECK(itdb_init_ipod(root, "MB565", "TestPod", &err));

    Itdb_iTunesDB *db = itdb_parse(root, &err);
    CHECK(db);
    const Itdb_IpodInfo *info = itdb_device_get_ipod_info(db->device);
    CHECK(info && (info->ipod_generation == ITDB_IPOD_GENERATION_CLASSIC_1 ||
                   info->ipod_generation == ITDB_IPOD_GENERATION_CLASSIC_2 ||
                   info->ipod_generation == ITDB_IPOD_GENERATION_CLASSIC_3));

    Itdb_Playlist *mpl = itdb_playlist_mpl(db);
    Itdb_Playlist *pl = itdb_playlist_new("Mix", FALSE);
    itdb_playlist_add(db, pl, -1);
    for (int i = 0; i < 3; i++) {
        Itdb_Track *t = itdb_track_new();
        t->title = g_strdup_printf("Song %d", i);
        t->artist = g_strdup("Artist");
        t->album = g_strdup("Album");
        t->tracklen = 180000;
        t->ipod_path = g_strdup_printf(":iPod_Control:Music:F00:TEST%d.mp3", i);
        itdb_track_add(db, t, -1);
        itdb_playlist_add_track(mpl, t, -1);
        if (i != 1) itdb_playlist_add_track(pl, t, -1);
    }
    CHECK(itdb_write(db, &err));
    itdb_free(db);

    db = itdb_parse(root, &err);
    CHECK(db);
    CHECK(g_list_length(db->tracks) == 3);
    Itdb_Playlist *mix = itdb_playlist_by_name(db, "Mix");
    CHECK(mix && g_list_length(mix->members) == 2);
    Itdb_Track *first = db->tracks->data;
    CHECK(first->title && g_str_has_prefix(first->title, "Song"));

    /* removal must leave a writable, consistent database */
    Itdb_Track *victim = db->tracks->data;
    itdb_playlist_remove_track(NULL, victim);
    itdb_track_unlink(victim);
    itdb_track_free(victim);
    CHECK(itdb_write(db, &err));
    itdb_free(db);
    db = itdb_parse(root, &err);
    CHECK(db && g_list_length(db->tracks) == 2);
    itdb_free(db);
    printf("roundtrip OK\n");
    return 0;
}
