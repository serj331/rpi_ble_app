#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <gio/gio.h>

#include "bcm_pwm.h"
#include "bcm_gpio.h"

#define PWM_PIN (12)
#define LED_PIN (24)
#define BUTTON_PIN (26)

const gchar* OBJMANAGER_PATH = "/bleapp";
const gchar* LEADVER_PATH = "/bleapp/adver";

const gchar* SERVC0_PATH = "/bleapp/servc0";
const gchar* CHARC0_PATH = "/bleapp/servc0/charc0";
const gchar* CHARC1_PATH = "/bleapp/servc0/charc1";

const gchar* BLE_DEVICE_NAME = "rpi_ble";
const gchar* SERVC0_UUID = "53790001-199b-67b9-6642-25a6df45a57e";
const gchar* CHARC0_UUID = "53790101-199b-67b9-6642-25a6df45a57e";
const gchar* CHARC1_UUID = "53790102-199b-67b9-6642-25a6df45a57e";

GDBusConnection* bus_conn;

volatile guint8 charc0_value = 0xF7;
volatile guint8 charc1_value = 0x00;

gboolean charc0_notify_en = FALSE;
gboolean charc1_notify_en = FALSE;

void on_charc0_value_change();
void on_charc1_value_change();

void vt_method_call_impl(GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name, const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data) {
    if(g_strcmp0(interface_name, "org.bluez.LEAdvertisement1") == 0) {
        if(g_strcmp0(method_name, "Release") == 0) {
            if(g_strcmp0(object_path, LEADVER_PATH) == 0) {
            }
        }
    } else if(g_strcmp0(interface_name, "org.bluez.GattCharacteristic1") == 0) {
        if(g_strcmp0(method_name, "ReadValue") == 0) {
            GVariantIter* args_iter;
            GVariant* opt_value;
            gchar* opt_key;
            g_variant_get(parameters, "(a{sv})", &args_iter);
            /*
            printf("ReadValue() call with opts: ");
            while(g_variant_iter_loop(args_iter, "{&sv}", &opt_key, &opt_value)) {
                printf(" %s ", opt_key);
            }
            printf("\n");
            */
            GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("ay"));
            if(g_strcmp0(object_path, CHARC0_PATH) == 0) {
                g_variant_builder_add (builder, "y", charc0_value);
                printf("read value=0x%X of charc0\n", charc0_value);
            } else if(g_strcmp0(object_path, CHARC1_PATH) == 0) {
                g_variant_builder_add (builder, "y", charc1_value);
                printf("read value=0x%X of charc1\n", charc1_value);
            }
            GVariant *value = g_variant_new ("(ay)", builder);
            g_variant_builder_unref (builder);
            g_dbus_method_invocation_return_value(invocation, value);
            return;
        } else if(g_strcmp0(method_name, "WriteValue") == 0) {
            GVariantIter* vals_iter;
            GVariantIter* opts_iter;
            guint8 new_val;
            GVariant* opt_value;
            gchar* opt_key;
            g_variant_get(parameters, "(aya{sv})",&vals_iter, &opts_iter);
            if(g_variant_iter_loop(vals_iter, "y", &new_val)) {
                if(g_strcmp0(object_path, CHARC0_PATH) == 0) {
                    charc0_value = new_val;
                    on_charc0_value_change();
                    printf("write new value=0x%X to charc0\n", new_val);
                } else if(g_strcmp0(object_path, CHARC1_PATH) == 0) {
                    charc1_value = new_val;
                    on_charc1_value_change();
                    printf("write new value=0x%X to charc1\n", new_val);
                }
            }
            /*
            printf("WriteValue() call with opts: ");
            while(g_variant_iter_loop(opts_iter, "{&sv}", &opt_key, &opt_value)) {
                printf(" %s ", opt_key);
            }
            printf("\n");
            */
        } else if(g_strcmp0(method_name, "StartNotify") == 0) {
            if(g_strcmp0(object_path, CHARC0_PATH) == 0) {
                charc0_notify_en = TRUE;
                puts("charc0 notify enabled");
            } else if(g_strcmp0(object_path, CHARC1_PATH) == 0) {
                charc1_notify_en = TRUE;
                puts("charc1 notify enabled");
            }
        } else if(g_strcmp0(method_name, "StopNotify") == 0) {
            if(g_strcmp0(object_path, CHARC0_PATH) == 0) {
                charc0_notify_en = FALSE;
                puts("charc0 notify disabled");
            } else if(g_strcmp0(object_path, CHARC1_PATH) == 0) {
                charc1_notify_en = FALSE;
                puts("charc1 notify disabled");
            }
        }
    }
    g_dbus_method_invocation_return_value(invocation, NULL); // empty return ?
}

GVariant* vt_get_property_impl(GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name, const gchar *property_name, GError **error, gpointer user_data) {
    if(g_strcmp0(interface_name, "org.bluez.LEAdvertisement1") == 0) {
        if(g_strcmp0(property_name, "Type") == 0) {
            return g_variant_new_string("peripheral");
        } else if(g_strcmp0(property_name, "ServiceUUIDs") == 0) {
            GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
            g_variant_builder_add (builder, "s", SERVC0_UUID);
            GVariant* ret_value = g_variant_new ("as", builder);
            g_variant_builder_unref (builder);
            return ret_value;
        } else if(g_strcmp0(property_name, "LocalName") == 0) {
            return g_variant_new_string(BLE_DEVICE_NAME);
        }
    } else if(g_strcmp0(interface_name, "org.bluez.GattService1") == 0) {
        if(g_strcmp0(property_name, "UUID") == 0) {
            return g_variant_new_string(SERVC0_UUID);
        } else if(g_strcmp0(property_name, "Primary") == 0) {
            return g_variant_new_boolean(TRUE);
        } 
    } else if(g_strcmp0(interface_name, "org.bluez.GattCharacteristic1") == 0) {
        if(g_strcmp0(property_name, "UUID") == 0) {
            if(g_strcmp0(object_path, CHARC0_PATH) == 0) {
                return g_variant_new_string(CHARC0_UUID);
            } else if(g_strcmp0(object_path, CHARC1_PATH) == 0) {
                return g_variant_new_string(CHARC1_UUID);
            }
            return g_variant_new_string("");
        } else if(g_strcmp0(property_name, "Service") == 0) {
            if(g_strcmp0(object_path, CHARC0_PATH) == 0 || g_strcmp0(object_path, CHARC1_PATH) == 0) {
                return g_variant_new_object_path(SERVC0_PATH);
            }
            return g_variant_new_object_path("");
        } else if(g_strcmp0(property_name, "Flags") == 0) {
            GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
            if(g_strcmp0(object_path, CHARC0_PATH) == 0 || g_strcmp0(object_path, CHARC1_PATH) == 0) {
                g_variant_builder_add(builder, "s", "read");
                g_variant_builder_add(builder, "s", "write");
                g_variant_builder_add(builder, "s", "notify");
            } 
            GVariant* ret_value = g_variant_new ("as", builder);
            g_variant_builder_unref(builder);
            return ret_value;
        } else if(g_strcmp0(property_name, "Value") == 0) {
            GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("ay"));
            if(g_strcmp0(object_path, CHARC0_PATH) == 0) {
                g_variant_builder_add (builder, "y", charc0_value);
            } else if(g_strcmp0(object_path, CHARC1_PATH) == 0) {
                g_variant_builder_add (builder, "y", charc1_value);
            }
            GVariant* ret_value = g_variant_new ("ay", builder);
            g_variant_builder_unref(builder);
            return ret_value;
        } 
    }
    return NULL;
}

gboolean vt_set_property_impl(GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name, const gchar *property_name, GVariant *value, GError **error, gpointer user_data) {
    return FALSE;
}
typedef struct {
    GDBusInterfaceSkeleton parent;
} GattCharc1Iface;

typedef struct {
    GDBusInterfaceSkeletonClass parent_class;
} GattCharc1IfaceClass;

G_DEFINE_TYPE(GattCharc1Iface, gattcharc1_iface, G_TYPE_DBUS_INTERFACE_SKELETON)

GDBusInterfaceInfo* gattcharc1_get_info(GDBusInterfaceSkeleton *skeleton) {
    static GDBusArgInfo dict_opts_arg = {
        -1,
        "options",
        "a{sv}",
        NULL
    };
    static GDBusArgInfo ay_arg = {
        -1,
        NULL,
        "ay",
        NULL
    };
    static GDBusPropertyInfo uuid_prop = {
        -1,
        "UUID",
        "s",
        G_DBUS_PROPERTY_INFO_FLAGS_READABLE, 
        NULL
    };
    static GDBusPropertyInfo service_prop = {
        -1,
        "Service",
        "o",
        G_DBUS_PROPERTY_INFO_FLAGS_READABLE, 
        NULL
    };
    static GDBusPropertyInfo value_prop = {
        -1,
        "Value",
        "ay",
        G_DBUS_PROPERTY_INFO_FLAGS_READABLE, 
        NULL
    };
    static GDBusPropertyInfo flags_prop = {
        -1,
        "Flags",
        "as",
        G_DBUS_PROPERTY_INFO_FLAGS_READABLE, 
        NULL
    };
    static GDBusArgInfo* rvm_in_args[] = {&dict_opts_arg, NULL};
    static GDBusArgInfo* rvm_out_args[] = {&ay_arg, NULL};
    static GDBusMethodInfo read_value_method = {
        -1,
        "ReadValue",
        rvm_in_args,
        rvm_out_args,
        NULL
    };
    static GDBusArgInfo* wvm_in_args[] = {&ay_arg, &dict_opts_arg, NULL};
    static GDBusMethodInfo write_value_method = {
        -1,
        "WriteValue",
        wvm_in_args,
        NULL,
        NULL
    };
    static GDBusMethodInfo start_noty_method = {
        -1,
        "StartNotify",
        NULL,
        NULL,
        NULL
    };
    static GDBusMethodInfo stop_noty_method = {
        -1,
        "StopNotify",
        NULL,
        NULL,
        NULL
    };
    static GDBusMethodInfo* iface_methods[] = {&read_value_method, &write_value_method, &start_noty_method, &stop_noty_method, NULL};
    static GDBusPropertyInfo* iface_props[] = {&uuid_prop, &service_prop, &value_prop, &flags_prop, NULL};
    static GDBusInterfaceInfo iface_info = {
        -1,
        "org.bluez.GattCharacteristic1",
        iface_methods,
        NULL,
        iface_props,
        NULL
    };
    return &iface_info;
}

//--------------------------------------------------------------------------------------------------------------

typedef struct {
    GDBusInterfaceSkeleton parent;
} GattServ1Iface;

typedef struct {
    GDBusInterfaceSkeletonClass parent_class;
} GattServ1IfaceClass;

G_DEFINE_TYPE(GattServ1Iface, gattserv1_iface, G_TYPE_DBUS_INTERFACE_SKELETON)

GDBusInterfaceInfo* gattserv1_get_info(GDBusInterfaceSkeleton *skeleton) {
    static GDBusPropertyInfo uuid_prop = {
        -1,
        "UUID",
        "s",
        G_DBUS_PROPERTY_INFO_FLAGS_READABLE, 
        NULL
    };
    static GDBusPropertyInfo primary_prop = {
        -1,
        "Primary",
        "b",
        G_DBUS_PROPERTY_INFO_FLAGS_READABLE, 
        NULL
    };
    static GDBusMethodInfo* iface_methods[] = {NULL};
    static GDBusPropertyInfo* iface_props[] = {&uuid_prop, &primary_prop, NULL};
    static GDBusInterfaceInfo iface_info = {
        -1,
        "org.bluez.GattService1",
        iface_methods,
        NULL,
        iface_props,
        NULL
    };
    return &iface_info;
}

//--------------------------------------------------------------------------------------------------------------

typedef struct {
    GDBusInterfaceSkeleton parent;
} LEAdver1Iface;

typedef struct {
    GDBusInterfaceSkeletonClass parent_class;
} LEAdver1IfaceClass;

G_DEFINE_TYPE(LEAdver1Iface, leadver1_iface, G_TYPE_DBUS_INTERFACE_SKELETON)

GDBusInterfaceInfo* leadver1_get_info(GDBusInterfaceSkeleton *skeleton) {
    static GDBusPropertyInfo type_prop = {
        -1,
        "Type",
        "s",
        G_DBUS_PROPERTY_INFO_FLAGS_READABLE, 
        NULL
    };
    static GDBusPropertyInfo serviceuuids_prop = {
        -1,
        "ServiceUUIDs",
        "as",
        G_DBUS_PROPERTY_INFO_FLAGS_READABLE, 
        NULL
    };
    static GDBusPropertyInfo localname_prop = {
        -1,
        "LocalName",
        "s",
        G_DBUS_PROPERTY_INFO_FLAGS_READABLE, 
        NULL
    };
    static GDBusMethodInfo release_method = {
        -1,
        "Release",
        NULL,
        NULL,
        NULL
    };
    static GDBusMethodInfo* iface_methods[] = {&release_method, NULL};
    static GDBusPropertyInfo* iface_props[] = {&type_prop, &serviceuuids_prop, &localname_prop, NULL};
    static GDBusInterfaceInfo iface_info = {
        -1,
        "org.bluez.LEAdvertisement1",
        iface_methods,
        NULL,
        iface_props,
        NULL
    };
    return &iface_info;
}

GVariant* iface_get_properties(GDBusInterfaceSkeleton *skeleton) {
   GVariantBuilder builder;
   GDBusInterfaceInfo *info;
   GDBusInterfaceVTable *vtable;
   guint n;
 
   /* Groan, this is completely generic code and should be in gdbus */
   info = g_dbus_interface_skeleton_get_info (skeleton);
   vtable = g_dbus_interface_skeleton_get_vtable (skeleton);
   g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));


   for (n = 0; info->properties[n] != NULL; n++)
     {
       if (info->properties[n]->flags & G_DBUS_PROPERTY_INFO_FLAGS_READABLE)
         {
           GVariant *value;
           g_return_val_if_fail (vtable->get_property != NULL, NULL);
          value = (vtable->get_property) (g_dbus_interface_skeleton_get_connection (skeleton), NULL,
                                           g_dbus_interface_skeleton_get_object_path (skeleton),
                                           info->name, info->properties[n]->name,
                                          NULL, skeleton);
           if (value != NULL)
             {
               g_variant_take_ref (value);
               g_variant_builder_add (&builder, "{sv}", info->properties[n]->name, value);
               g_variant_unref (value);
             }
         }
     }
    return g_variant_builder_end (&builder);
}

volatile GattCharc1Iface* charc0_iface;

void iface_flush(GDBusInterfaceSkeleton *skeleton) {
    if(skeleton == (GDBusInterfaceSkeleton *)charc0_iface) {
        GVariantBuilder* signal_params = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
        //-----------------------------------------------------------------------------
        GVariantBuilder* charc0_val_builder = g_variant_builder_new(G_VARIANT_TYPE ("ay"));
        g_variant_builder_add (charc0_val_builder, "y", charc0_value);
        GVariant* value  = g_variant_new ("ay", charc0_val_builder);
        g_variant_builder_unref(charc0_val_builder);
        //-----------------------------------------------------------------------------
        g_variant_builder_add(signal_params, "{sv}", "Value", value);

        if(g_dbus_connection_emit_signal(bus_conn, NULL, CHARC0_PATH, "org.freedesktop.DBus.Properties", "PropertiesChanged", g_variant_new("(sa{sv}as)","org.bluez.GattCharacteristic1",signal_params,NULL), NULL) == FALSE) {
            puts("failed to emmit PropertiesChanged signal!");
        }
        g_variant_builder_unref(signal_params);
    }
}

GDBusInterfaceVTable* iface_get_vtable(GDBusInterfaceSkeleton *skeleton) {
    static GDBusInterfaceVTable vtable = {
        vt_method_call_impl,
        vt_get_property_impl,
        vt_set_property_impl
    };
    return &vtable;
}

void leadver1_iface_init(LEAdver1Iface* iface) {
}

void leadver1_iface_class_init(LEAdver1IfaceClass *class) {
    GDBusInterfaceSkeletonClass *skeleton_class = G_DBUS_INTERFACE_SKELETON_CLASS(class);
    skeleton_class->get_info = leadver1_get_info;
    skeleton_class->get_properties = iface_get_properties;
    skeleton_class->flush = iface_flush;
    skeleton_class->get_vtable = iface_get_vtable;
}

void gattserv1_iface_init(GattServ1Iface* iface) {
}

void gattserv1_iface_class_init(GattServ1IfaceClass *class) {
    GDBusInterfaceSkeletonClass *skeleton_class = G_DBUS_INTERFACE_SKELETON_CLASS(class);
    skeleton_class->get_info = gattserv1_get_info;
    skeleton_class->get_properties = iface_get_properties;
    skeleton_class->flush = iface_flush;
    skeleton_class->get_vtable = iface_get_vtable;
}

void gattcharc1_iface_init(GattCharc1Iface* iface) {
}

void gattcharc1_iface_class_init(GattCharc1IfaceClass *class) {
    GDBusInterfaceSkeletonClass *skeleton_class = G_DBUS_INTERFACE_SKELETON_CLASS(class);
    skeleton_class->get_info = gattcharc1_get_info;
    skeleton_class->get_properties = iface_get_properties;
    skeleton_class->flush = iface_flush;
    skeleton_class->get_vtable = iface_get_vtable;
}

void reg_app_done(GObject *source_object, GAsyncResult *res, gpointer user_data) {
}

void reg_adv_done(GObject *source_object, GAsyncResult *res, gpointer user_data) {
}

void on_charc0_value_change() {
    if(charc0_value == 0xa6) {
        gpio_writepin(LED_PIN, 1);
    } else if(charc0_value == 0xf7) {
        gpio_writepin(LED_PIN, 0);
    }
}

void on_charc1_value_change() {
    pwm_set_ch1(64, charc1_value & 63);
}

void* button_handler(void* args) {
    while(1) {
        usleep(50000);
        if(gpio_readpin(BUTTON_PIN) == 0) {
            if(charc0_value == 0xa6) {
                charc0_value = 0xf7;
            } else if(charc0_value == 0xf7) {
                charc0_value = 0xa6;
            }
            if(charc0_notify_en) {
                g_dbus_interface_skeleton_flush(G_DBUS_INTERFACE_SKELETON(charc0_iface));
            }
            on_charc0_value_change();
            while(gpio_readpin(BUTTON_PIN) == 0) usleep(50000);
        }
    }
}

int main() {
    if(gpio_access() != 0) {
        puts("failed to access gpio module.");
        return -1;
    }

    if(pwm_init(1,0) != 0) {
        puts("failed to init pwm module.");
        return -1;
    }

    gpio_confpin(PWM_PIN, FSEL_ALT0,PUD_OFF); //PWM CH1
    gpio_confpin(LED_PIN, FSEL_OUT,PUD_OFF);
    gpio_confpin(BUTTON_PIN, FSEL_IN,PUD_UP);

    GMainLoop * loop = g_main_loop_new(NULL, FALSE);

    bus_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
    g_assert(bus_conn != NULL);

    puts("connected to dbus");

    GDBusObjectManagerServer* manager = g_dbus_object_manager_server_new(OBJMANAGER_PATH);
    g_assert(manager != NULL);

    //add advertiser object
    GDBusObjectSkeleton* leadver_obj = g_dbus_object_skeleton_new(LEADVER_PATH);
    g_assert(leadver_obj != NULL);
    LEAdver1Iface* leadver_iface = g_object_new(leadver1_iface_get_type(), NULL);
    g_assert(leadver_iface != NULL);
    g_dbus_object_skeleton_add_interface(leadver_obj, G_DBUS_INTERFACE_SKELETON(leadver_iface));
    g_dbus_object_manager_server_export(manager, leadver_obj);

    //add gatt service0 object
    GDBusObjectSkeleton* service0_obj = g_dbus_object_skeleton_new(SERVC0_PATH);
    g_assert(service0_obj != NULL);
    GattServ1Iface* servc0_iface = g_object_new(gattserv1_iface_get_type(), NULL);
    g_assert(servc0_iface != NULL);
    g_dbus_object_skeleton_add_interface(service0_obj, G_DBUS_INTERFACE_SKELETON(servc0_iface)); 
    g_dbus_object_manager_server_export(manager, service0_obj);

    //add gatt charc0 object
    GDBusObjectSkeleton* charc0_obj = g_dbus_object_skeleton_new(CHARC0_PATH);
    g_assert(charc0_obj != NULL);
    charc0_iface = g_object_new(gattcharc1_iface_get_type(), NULL);
    g_assert(charc0_iface != NULL);
    g_dbus_object_skeleton_add_interface(charc0_obj, G_DBUS_INTERFACE_SKELETON(charc0_iface)); 
    g_dbus_object_manager_server_export(manager, charc0_obj);

    //add gatt charc1 object
    GDBusObjectSkeleton* charc1_obj = g_dbus_object_skeleton_new(CHARC1_PATH);
    g_assert(charc1_obj != NULL);
    GattCharc1Iface* charc1_iface = g_object_new(gattcharc1_iface_get_type(), NULL);
    g_assert(charc1_iface != NULL);
    g_dbus_object_skeleton_add_interface(charc1_obj, G_DBUS_INTERFACE_SKELETON(charc1_iface)); 
    g_dbus_object_manager_server_export(manager, charc1_obj);

    //link connection to manager
    g_dbus_object_manager_server_set_connection(manager, bus_conn);

    //register object manager
    GVariant* regapp_args = g_variant_new ("(oa{sv})", OBJMANAGER_PATH, NULL);    
    g_dbus_connection_call(bus_conn, "org.bluez", "/org/bluez/hci0", "org.bluez.GattManager1", "RegisterApplication", regapp_args, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, reg_app_done, NULL);

    //register ble advertiser
    GVariant* regadv_args = g_variant_new ("(oa{sv})", LEADVER_PATH, NULL);    
    g_dbus_connection_call(bus_conn, "org.bluez", "/org/bluez/hci0", "org.bluez.LEAdvertisingManager1", "RegisterAdvertisement", regadv_args, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, reg_adv_done, NULL);

    gpio_writepin(LED_PIN, 1);
    sleep(1);
    gpio_writepin(LED_PIN, 0);

    pthread_t button_thread;
    if(pthread_create(&button_thread, NULL, button_handler, NULL) != 0) {
        puts("failed to create button handling thread.");
        return -1;
    }
  
    puts("OK-> loop_run()");

    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    return 0;
}
