class animations {
  public:
    static const unsigned char* get_next_animation();

    static const unsigned char* get_back_animation(unsigned int index);

    static const unsigned char* get_pause_animation(unsigned int index);

    static const unsigned char* get_pause_icon();
};