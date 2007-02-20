/** \file
 * SPIcon: Generic icon widget
 */
/*
 * Copyright (C) 2007 Bryce W. Harrington <bryce@bryceharrington.org>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 *
 */

GdkPixbuf* get_pixbuf(GdkPixbuf* pixbuf) {
    return pixbuf;
}

GdkPixbuf* render_pixbuf(NRArenaItem* root, double scale_factor, const NR::Rect& dbox, unsigned psize) {
    NRGC gc(NULL);
    NRMatrix t;

    nr_matrix_set_scale(&t, scale_factor, scale_factor);
    nr_arena_item_set_transform(root, &t);

    nr_matrix_set_identity(&gc.transform);
    nr_arena_item_invoke_update( root, NULL, &gc,
                                 NR_ARENA_ITEM_STATE_ALL,
                                 NR_ARENA_ITEM_STATE_NONE );

    /* Item integer bbox in points */
    NRRectL ibox;
    ibox.x0 = (int) floor(scale_factor * dbox.min()[NR::X] + 0.5);
    ibox.y0 = (int) floor(scale_factor * dbox.min()[NR::Y] + 0.5);
    ibox.x1 = (int) floor(scale_factor * dbox.max()[NR::X] + 0.5);
    ibox.y1 = (int) floor(scale_factor * dbox.max()[NR::Y] + 0.5);

    /* Find visible area */
    int width = ibox.x1 - ibox.x0;
    int height = ibox.y1 - ibox.y0;
    int dx = psize;
    int dy = psize;
    dx = (dx - width)/2; // watch out for size, since 'unsigned'-'signed' can cause problems if the result is negative
    dy = (dy - height)/2;

    NRRectL area;
    area.x0 = ibox.x0 - dx;
    area.y0 = ibox.y0 - dy;
    area.x1 = area.x0 + psize;
    area.y1 = area.y0 + psize;

    /* Actual renderable area */
    NRRectL ua;
    ua.x0 = std::max(ibox.x0, area.x0);
    ua.y0 = std::max(ibox.y0, area.y0);
    ua.x1 = std::min(ibox.x1, area.x1);
    ua.y1 = std::min(ibox.y1, area.y1);

    /* Set up pixblock */
    guchar *px = g_new(guchar, 4 * psize * psize);
    memset(px, 0x00, 4 * psize * psize);

    /* Render */
    NRPixBlock B;
    nr_pixblock_setup_extern( &B, NR_PIXBLOCK_MODE_R8G8B8A8N,
                              ua.x0, ua.y0, ua.x1, ua.y1,
                              px + 4 * psize * (ua.y0 - area.y0) +
                              4 * (ua.x0 - area.x0),
                              4 * psize, FALSE, FALSE );
    nr_arena_item_invoke_render( root, &ua, &B,
                                 NR_ARENA_ITEM_RENDER_NO_CACHE );
    nr_pixblock_release(&B);

    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_data(px,
                                      GDK_COLORSPACE_RGB,
                                      TRUE,
                                      8, psize, psize, psize * 4,
                                      (GdkPixbufDestroyNotify)g_free,
                                      NULL);

    return pixbuf;
}
