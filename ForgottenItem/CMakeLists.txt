cmake_minimum_required(VERSION 3.22.1)

project("ForgottenItem")

add_terraria_mod(ForgottenItem
        mod.c
        item_id.c
        item.c
        item_drop_database.c
        prefix_legacy.c
        recipe.c
)