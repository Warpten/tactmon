PGDMP     !            
        {           tactmon    15.1    15.1 *    '           0    0    ENCODING    ENCODING        SET client_encoding = 'UTF8';
                      false            (           0    0 
   STDSTRINGS 
   STDSTRINGS     (   SET standard_conforming_strings = 'on';
                      false            )           0    0 
   SEARCHPATH 
   SEARCHPATH     8   SELECT pg_catalog.set_config('search_path', '', false);
                      false            *           1262    16398    tactmon    DATABASE     �   CREATE DATABASE tactmon WITH TEMPLATE = template0 ENCODING = 'UTF8' LOCALE_PROVIDER = libc LOCALE = 'English_United States.1252';
    DROP DATABASE tactmon;
                postgres    false            +           0    0    DATABASE tactmon    COMMENT     I   COMMENT ON DATABASE tactmon IS 'Stores data related to tact monitoring';
                   postgres    false    3370            �            1259    16608    bound_channels    TABLE     �   CREATE TABLE public.bound_channels (
    id integer NOT NULL,
    guild_id bigint NOT NULL,
    channel_id bigint NOT NULL,
    product_name text NOT NULL
);
 "   DROP TABLE public.bound_channels;
       public         heap    postgres    false            �            1259    16607    bound_channels_id_seq    SEQUENCE     �   CREATE SEQUENCE public.bound_channels_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
 ,   DROP SEQUENCE public.bound_channels_id_seq;
       public          postgres    false    219            ,           0    0    bound_channels_id_seq    SEQUENCE OWNED BY     O   ALTER SEQUENCE public.bound_channels_id_seq OWNED BY public.bound_channels.id;
          public          postgres    false    218            �            1259    16400    builds    TABLE     �   CREATE TABLE public.builds (
    id integer NOT NULL,
    build_name text NOT NULL,
    build_config text NOT NULL,
    cdn_config text NOT NULL,
    product_name text,
    detected_at bigint DEFAULT 0 NOT NULL,
    region text NOT NULL
);
    DROP TABLE public.builds;
       public         heap    postgres    false            -           0    0    COLUMN builds.detected_at    COMMENT     �   COMMENT ON COLUMN public.builds.detected_at IS 'std::chrono::system_clock::time_point::count() at which the build was detected.';
          public          postgres    false    215            �            1259    16399    builds_id_seq    SEQUENCE     �   CREATE SEQUENCE public.builds_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
 $   DROP SEQUENCE public.builds_id_seq;
       public          postgres    false    215            .           0    0    builds_id_seq    SEQUENCE OWNED BY     ?   ALTER SEQUENCE public.builds_id_seq OWNED BY public.builds.id;
          public          postgres    false    214            �            1259    17178    command_states    TABLE     �   CREATE TABLE public.command_states (
    id integer NOT NULL,
    name text NOT NULL,
    hash bigint NOT NULL,
    version integer NOT NULL
);
 "   DROP TABLE public.command_states;
       public         heap    postgres    false            �            1259    17177    command_states_id_seq    SEQUENCE     �   CREATE SEQUENCE public.command_states_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
 ,   DROP SEQUENCE public.command_states_id_seq;
       public          postgres    false    223            /           0    0    command_states_id_seq    SEQUENCE OWNED BY     O   ALTER SEQUENCE public.command_states_id_seq OWNED BY public.command_states.id;
          public          postgres    false    222            �            1259    16414    products_id_seq    SEQUENCE     �   CREATE SEQUENCE public.products_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
 &   DROP SEQUENCE public.products_id_seq;
       public          postgres    false            �            1259    16415    products    TABLE     �   CREATE TABLE public.products (
    id integer DEFAULT nextval('public.products_id_seq'::regclass) NOT NULL,
    name text NOT NULL,
    sequence_id bigint NOT NULL
);
    DROP TABLE public.products;
       public         heap    postgres    false    216            �            1259    17169    tracked_files    TABLE     �   CREATE TABLE public.tracked_files (
    id integer NOT NULL,
    product_name text,
    file_path text,
    display_name text
);
 !   DROP TABLE public.tracked_files;
       public         heap    postgres    false            �            1259    17168    tracked_file_id_seq    SEQUENCE     �   CREATE SEQUENCE public.tracked_file_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
 *   DROP SEQUENCE public.tracked_file_id_seq;
       public          postgres    false    221            0           0    0    tracked_file_id_seq    SEQUENCE OWNED BY     L   ALTER SEQUENCE public.tracked_file_id_seq OWNED BY public.tracked_files.id;
          public          postgres    false    220            |           2604    16611    bound_channels id    DEFAULT     v   ALTER TABLE ONLY public.bound_channels ALTER COLUMN id SET DEFAULT nextval('public.bound_channels_id_seq'::regclass);
 @   ALTER TABLE public.bound_channels ALTER COLUMN id DROP DEFAULT;
       public          postgres    false    218    219    219            y           2604    16403 	   builds id    DEFAULT     f   ALTER TABLE ONLY public.builds ALTER COLUMN id SET DEFAULT nextval('public.builds_id_seq'::regclass);
 8   ALTER TABLE public.builds ALTER COLUMN id DROP DEFAULT;
       public          postgres    false    214    215    215            ~           2604    17181    command_states id    DEFAULT     v   ALTER TABLE ONLY public.command_states ALTER COLUMN id SET DEFAULT nextval('public.command_states_id_seq'::regclass);
 @   ALTER TABLE public.command_states ALTER COLUMN id DROP DEFAULT;
       public          postgres    false    223    222    223            }           2604    17172    tracked_files id    DEFAULT     s   ALTER TABLE ONLY public.tracked_files ALTER COLUMN id SET DEFAULT nextval('public.tracked_file_id_seq'::regclass);
 ?   ALTER TABLE public.tracked_files ALTER COLUMN id DROP DEFAULT;
       public          postgres    false    221    220    221                       0    16608    bound_channels 
   TABLE DATA           P   COPY public.bound_channels (id, guild_id, channel_id, product_name) FROM stdin;
    public          postgres    false    219   �-                 0    16400    builds 
   TABLE DATA           m   COPY public.builds (id, build_name, build_config, cdn_config, product_name, detected_at, region) FROM stdin;
    public          postgres    false    215   �-       $          0    17178    command_states 
   TABLE DATA           A   COPY public.command_states (id, name, hash, version) FROM stdin;
    public          postgres    false    223   �/                 0    16415    products 
   TABLE DATA           9   COPY public.products (id, name, sequence_id) FROM stdin;
    public          postgres    false    217   �/       "          0    17169    tracked_files 
   TABLE DATA           R   COPY public.tracked_files (id, product_name, file_path, display_name) FROM stdin;
    public          postgres    false    221   98       1           0    0    bound_channels_id_seq    SEQUENCE SET     C   SELECT pg_catalog.setval('public.bound_channels_id_seq', 1, true);
          public          postgres    false    218            2           0    0    builds_id_seq    SEQUENCE SET     <   SELECT pg_catalog.setval('public.builds_id_seq', 35, true);
          public          postgres    false    214            3           0    0    command_states_id_seq    SEQUENCE SET     D   SELECT pg_catalog.setval('public.command_states_id_seq', 1, false);
          public          postgres    false    222            4           0    0    products_id_seq    SEQUENCE SET     ?   SELECT pg_catalog.setval('public.products_id_seq', 278, true);
          public          postgres    false    216            5           0    0    tracked_file_id_seq    SEQUENCE SET     A   SELECT pg_catalog.setval('public.tracked_file_id_seq', 2, true);
          public          postgres    false    220            �           2606    16615 "   bound_channels bound_channels_pkey 
   CONSTRAINT     l   ALTER TABLE ONLY public.bound_channels
    ADD CONSTRAINT bound_channels_pkey PRIMARY KEY (id, channel_id);
 L   ALTER TABLE ONLY public.bound_channels DROP CONSTRAINT bound_channels_pkey;
       public            postgres    false    219    219            �           2606    16407    builds builds_pkey 
   CONSTRAINT     P   ALTER TABLE ONLY public.builds
    ADD CONSTRAINT builds_pkey PRIMARY KEY (id);
 <   ALTER TABLE ONLY public.builds DROP CONSTRAINT builds_pkey;
       public            postgres    false    215            �           2606    17187 &   command_states command_state_uniq_name 
   CONSTRAINT     a   ALTER TABLE ONLY public.command_states
    ADD CONSTRAINT command_state_uniq_name UNIQUE (name);
 P   ALTER TABLE ONLY public.command_states DROP CONSTRAINT command_state_uniq_name;
       public            postgres    false    223            �           2606    17185 "   command_states command_states_pkey 
   CONSTRAINT     `   ALTER TABLE ONLY public.command_states
    ADD CONSTRAINT command_states_pkey PRIMARY KEY (id);
 L   ALTER TABLE ONLY public.command_states DROP CONSTRAINT command_states_pkey;
       public            postgres    false    223            �           2606    17176 4   tracked_files pk_tracked_file_product_name_file_path 
   CONSTRAINT     �   ALTER TABLE ONLY public.tracked_files
    ADD CONSTRAINT pk_tracked_file_product_name_file_path UNIQUE (product_name, file_path);
 ^   ALTER TABLE ONLY public.tracked_files DROP CONSTRAINT pk_tracked_file_product_name_file_path;
       public            postgres    false    221    221            �           2606    16422    products products_pkey 
   CONSTRAINT     T   ALTER TABLE ONLY public.products
    ADD CONSTRAINT products_pkey PRIMARY KEY (id);
 @   ALTER TABLE ONLY public.products DROP CONSTRAINT products_pkey;
       public            postgres    false    217            �           1259    16408    ix_builds_product_name    INDEX     P   CREATE INDEX ix_builds_product_name ON public.builds USING hash (product_name);
 *   DROP INDEX public.ix_builds_product_name;
       public            postgres    false    215            �           1259    17063    ix_products_name    INDEX     L   CREATE UNIQUE INDEX ix_products_name ON public.products USING btree (name);
 $   DROP INDEX public.ix_products_name;
       public            postgres    false    217                   x������ � �         �  x�Ŗ�j�@���w��ٟٙ��ZB!7�;�iCBb��'�`�BV/�ߑt�������O��}��/p+����n��ݠc �S�S����'�L��g���HʬAJ%-�����?�B�1��yx܌ؗ��#�1�+�&T�a�b!@ֈ�X9T+E�r����3�>��#�ܰݍ�/q�sL}�OOc>6�{ݜύ8 3Iɤ)FlJ�`�
�w��u�3fA�u�6I �S�(�S�,�ݩM���&pwjN{D*�h�/we�������8SU.F�њ�>M����3v-�3_��Dq� �O�t��S �&��P��
�D�L|M(	��w�Y���h�y�d�&�?NZ N�~�� "�N$jV�cq�G��)b��F�{���Z=.�a.-�a��+��WCCa�D1��\����&�_�~���m8;��`���]��m�|Z4��ų���	Z��q����L      $      x������ � �         c  x�m�Yn#�E�3����� CC�Jh��d]��K�f��2�(�dZ�ݗ�|_RM1���_��}G8B]v����>@r�"�/���n�!��:´���}��B���%�!c����(���nDr_�g}9��1������ގח��t�_RO%����J|���o�Qzƹ���Q�kE�}���������{�v^/��~�en5�Җc�U������:CX߹���u=_Onɒb��5��x&ƒ+h"Us��a�Z���u�-#�$����v��^_�Қ�P�q}�v�Т�GRI6��H��>B"��ؕ��Kh�(��*Qq4Rh���.�ЉQmu"�5��ϡo�O�:��X=�N�e�g�N�7A[蔝r�!��ob��)9�l5t
N!��N�)#��)�&�C�/��z��^?��&S<S���|�[��_<[èO�n��F{���Z
�/_���4�F��>(hcB�i�N���嫅*����QH�Y�����Rj��z6["�J��݁8�s�%�{H�(�� �$IF��SAĈ��`�غ�2��$ŀ�8R9U���U�ȶ}�b'ے�KY�>}b�2�p�����D�x�̮ÃOD��]�_HD%�@J@Tb��g���^O:���]������^�{/H����D	��N�g�Gt�͠���@��s�GM0�EUѝ{�PS`��H��R��$���A�T��A�TRI�ʥ�Fı|�\�^ J���3!���"D8K�P���Xv�
#
G1�pC���P��P���8*��A����j�jm��Ć&�μ�k�V�Zor�KФ�fDw����׼�ᥚ�A~v��J�a���`�>iS��)`3N������� ��1Nd���9 ��T
�=>�|��oĭ�r�v<��]4C�ߴ������J����
�)<H�U�9 f��������j&$��7V�*,C�Ӧ���iS��ډ:e��K�f�-�r�V� ���`�dKZ��N4��|��̬��Oi�0#�_����KA�_�B�g���| ��F�~� �S�(�a2QӖ�H$ó!ݪwn�Jљɻ�� ��M�@���l�����{:��:u�)�IJu�r�LRJ*	��r\�]2!�1̊:-O$0���cw0d)��{ݝ�_�������ɸ��zG@�tXg[k�E����-7�� ��&w[k����<N鬒�u��
�nD/�8�1DRƼo�4���3q��s�Ϸ�?�%k����M�i�IP*ᖚ\2���N�DJ������a�9�0hP��O��9i�4(g�(�nGO[�v1�#�~Y�����=��/�R����O"���� ��}�6YRʭ3��"?c��$
���5ei���2|f��1�
�{�!r��>t�~,�-Y���i����mw��/�ug��Z�B�؜�'m�W �5�r{�ܽ|�Y1
���i�i�wQ?�lEF����JO�!z"�dx �Ǔ�/��+��(����P@KZ2�y���4!KΠ��ę��g���:�!�7�I��ŝJ!0�*�Q�S�����|XDU=p�.T4���t�k�b�Z�6T$�6E1uǧ:�"����v9�I!� ��|�c��o�y%���Oh�h�JY�=į࿮t)4m���J�5M��(�Z^������F����@��х�V�lᬑi'od�n�Դ)�U�}�4^Ň�S��{�N����Z�3��?�8+t���nTMbX�����X����Y������'�oϰ%#��y�$��zgffn�l.�@t!����:ʹ-���B���
3!���F%D_�c�����%�+}BG�XЮ�PS��ؠ��Eh�-�'��&e2(��)όאO��m�(D/V�ډ�Vx:��@�V�
��kd��@����A��5���$[���EE��@���
��+��,p��z���� ���`p2�S�I�Y۷�o���!�&c�H}c�����=)V	]zˬi�� Z-�o��gxg,�''P/�"my���M��W"�XN�s��F�Io��}�q�a�Q����sJ��h��d�y�L����}x�G��9,��N����X�6��_�_Pf?�      "      x�3�,�/��/�K�H��\1z\\\ u��     