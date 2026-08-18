package com.my.newproject;

import androidx.room.Dao;
import androidx.room.Insert;
import androidx.room.Query;
import java.util.List;

@Dao
public interface ImageDao {
    @Insert
    void insertImage(ImageEntity image);

    @Query("SELECT * FROM saved_images")
    List<ImageEntity> getAllImages();

    @Query("DELETE FROM saved_images WHERE id = :id")
    void deleteImageById(int id);
}
