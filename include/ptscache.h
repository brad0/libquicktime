typedef struct lqt_encoder_pts_cache_s
  {
  int64_t frame_counter;
  int num_frames;
  int frames_alloc;
  
  struct
    {
    int64_t pts;
    int64_t duration;
    int64_t frame_num;
    } * entries;
  } lqt_encoder_pts_cache_t;

lqt_encoder_pts_cache_t * lqt_encoder_pts_cache_create(void);

void lqt_encoder_pts_cache_destroy(lqt_encoder_pts_cache_t * c);

int
lqt_encoder_pts_cache_push_frame(lqt_encoder_pts_cache_t * c,
                                 int64_t pts, int64_t duration);

int
lqt_encoder_pts_cache_pop_packet(lqt_encoder_pts_cache_t * c,
                                 lqt_packet_t * p,
                                 int64_t frame_num, int64_t pts);
