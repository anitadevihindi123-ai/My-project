package com.my.newproject;

import androidx.room.Entity;
import androidx.room.PrimaryKey;

@Entity(tableName = "saved_images")
public class ImageEntity {
    @PrimaryKey(autoGenerate = true)
    public int id;

    public String filePath;
    public String type; // "image" या "video"
    public String duration; // वीडियो के लिए
}
