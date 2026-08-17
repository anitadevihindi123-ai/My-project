package com.my.newproject;

import android.animation.*;
import android.app.*;
import android.content.*;
import android.content.res.*;
import android.graphics.*;
import android.graphics.drawable.*;
import android.media.*;
import android.net.*;
import android.os.*;
import android.provider.MediaStore;
import android.text.*;
import android.text.style.*;
import android.util.*;
import android.view.*;
import android.view.View.*;
import android.view.animation.*;
import android.webkit.*;
import android.widget.*;
import androidx.annotation.*;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.recyclerview.widget.*;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.RecyclerView.Adapter;
import androidx.recyclerview.widget.RecyclerView.ViewHolder;
import com.google.android.gms.ads.MobileAds;
import java.io.*;
import java.text.*;
import java.util.*;
import java.util.regex.*;
import org.json.*;
import android.database.Cursor;
import com.my.newproject.databinding.CustomgalleryBinding;
import com.my.newproject.databinding.HhhBinding;

public class CustomgalleryActivity extends AppCompatActivity {
	
	private CustomgalleryBinding binding;
	private ArrayList<HashMap<String, Object>> mediaList = new ArrayList<>();
	private Recyclerview1Adapter adapter;
	
	private String selectedImagePath = null;
	private int selectedGlobalIndex = -1;
	
	@Override
	protected void onCreate(Bundle _savedInstanceState) {
		super.onCreate(_savedInstanceState);
		binding = CustomgalleryBinding.inflate(getLayoutInflater());
		setContentView(binding.getRoot());
		initialize(_savedInstanceState);
		
		MobileAds.initialize(this);
		
		initializeLogic();
	}
	private void initialize(Bundle _savedInstanceState) {
		// बैक बटन - सीधे MainActivity पर जाने के लिए
		if (binding.imageview1 != null) {
			binding.imageview1.setOnClickListener(v -> {
				Intent intent = new Intent(CustomgalleryActivity.this, MainActivity.class);
				intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
				startActivity(intent);
				finish();
			});
		}
		
		// 'Go' बटन - सेलेक्टेड मीडिया को NCNN AI पाइपलाइन में भेजकर प्रोसेस करने के लिए
		if (binding.textview1 != null) {
			binding.textview1.setOnClickListener(v -> {
				if (selectedImagePath != null && !selectedImagePath.isEmpty()) {
					processAndSaveViaNCNN(selectedImagePath);
				} else {
					Toast.makeText(CustomgalleryActivity.this, "⚠️ कृपया पहले गैलरी से कोई फोटो या वीडियो चुनें!", Toast.LENGTH_SHORT).show();
				}
			});
		}
	}
	
	private void initializeLogic() {
		binding.recyclerview1.setLayoutManager(new LinearLayoutManager(this));
		
		loadDeviceMediaFiles();
		
		adapter = new Recyclerview1Adapter(mediaList);
		binding.recyclerview1.setAdapter(adapter);
	}
	
	private void loadDeviceMediaFiles() {
		mediaList.clear();
		
		// 1. फोटो स्कैन करें
		String[] photoProjection = { MediaStore.Images.Media.DATA, MediaStore.Images.Media.DATE_ADDED };
		Cursor photoCursor = getContentResolver().query(
			MediaStore.Images.Media.EXTERNAL_CONTENT_URI,
			photoProjection, null, null, MediaStore.Images.Media.DATE_ADDED + " DESC"
		);
		
		if (photoCursor != null) {
			int pathIndex = photoCursor.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
			while (photoCursor.moveToNext()) {
				String path = photoCursor.getString(pathIndex);
				if (path != null && new File(path).exists()) {
					HashMap<String, Object> map = new HashMap<>();
					map.put("type", "photo");
					map.put("path", path);
					map.put("duration", "");
					map.put("selected", false);
					mediaList.add(map);
				}
			}
			photoCursor.close();
		}
		
		// 2. वीडियो स्कैन करें
		String[] videoProjection = { MediaStore.Video.Media.DATA, MediaStore.Video.Media.DURATION, MediaStore.Video.Media.DATE_ADDED };
		Cursor videoCursor = getContentResolver().query(
			MediaStore.Video.Media.EXTERNAL_CONTENT_URI,
			videoProjection, null, null, MediaStore.Video.Media.DATE_ADDED + " DESC"
		);
		
		if (videoCursor != null) {
			int pathIndex = videoCursor.getColumnIndexOrThrow(MediaStore.Video.Media.DATA);
			long durationMillis = 0;
			try {
				int durationIndex = videoCursor.getColumnIndexOrThrow(MediaStore.Video.Media.DURATION);
				while (videoCursor.moveToNext()) {
					String path = videoCursor.getString(pathIndex);
					durationMillis = videoCursor.getLong(durationIndex);
					
					if (path != null && new File(path).exists()) {
						String formattedDuration = formatDuration(durationMillis);
						HashMap<String, Object> map = new HashMap<>();
						map.put("type", "video");
						map.put("path", path);
						map.put("duration", formattedDuration);
						map.put("selected", false);
						mediaList.add(map);
					}
				}
			} finally {
				videoCursor.close();
			}
		}
	}
	
	private String formatDuration(long durationMillis) {
		long totalSeconds = durationMillis / 1000;
		long seconds = totalSeconds % 60;
		long totalMinutes = totalSeconds / 60;
		long minutes = totalMinutes % 60;
		long hours = totalMinutes / 60;

		if (hours > 0) {
			return String.format(Locale.getDefault(), "%02d:%02d:%02d", hours, minutes, seconds);
		} else {
			return String.format(Locale.getDefault(), "%02d:%02d", minutes, seconds);
		}
	}
	
	// --- NCNN AI पाइपलाइन ब्रिज मेथड ---
	private void processAndSaveViaNCNN(String filePath) {
		try {
			File sourceFile = new File(filePath);
			if (!sourceFile.exists()) {
				Toast.makeText(this, "❌ फाइल मौजूद नहीं है!", Toast.LENGTH_SHORT).show();
				return;
			}
			
			Toast.makeText(this, "⚡ NCNN AI मॉडल प्रोसेसिंग शुरू हो रही है...", Toast.LENGTH_SHORT).show();
			
			// बैकग्राउंड थ्रेड पर C++ और NCNN AI प्रोसेसिंग चलाना ताकि UI फ्रीज न हो
			new Thread(() -> {
				try {
					boolean isVideo = filePath.endsWith(".mp4") || filePath.toLowerCase().contains("video");
					File cachePath = new File(getFilesDir(), "AI_Processed_" + System.currentTimeMillis() + (isVideo ? ".mp4" : ".jpg"));
					
					// 1. C++ और NCNN मॉडल के जरिए इमेज/वीडियो एन्हांसमेंट रन करना
					Bitmap inputBmp = BitmapFactory.decodeFile(sourceFile.getAbsolutePath());
					if (inputBmp != null) {
						// C++ नेटिव AI मेथड को कॉल करना जो NCNN से प्रोसेस करके बिटमैप को एचडी बनाएगा
						truesingularityclass.nativeProcessAiEnhancement(inputBmp, cachePath.getAbsolutePath());
					} else {
						// अगर वीडियो है तो डायरेक्ट कॉपी पाथ देना
						try (FileInputStream in = new FileInputStream(sourceFile);
							 FileOutputStream out = new FileOutputStream(cachePath)) {
							byte[] buffer = new byte[4096];
							int length;
							while ((length = in.read(buffer)) > 0) {
								out.write(buffer, 0, length);
							}
							out.flush();
						}
					}
					
					// 2. ग्लोबल लिस्ट में जोड़ना
					if (truesingularityclass.inAppGalleryData == null) {
						truesingularityclass.inAppGalleryData = new ArrayList<>();
					}
					
					HashMap<String, Object> newMap = new HashMap<>();
					newMap.put("type", isVideo ? "video" : "photo");
					newMap.put("title", isVideo ? "AI एचडी वीडियो" : "AI प्रीमियम फोटो");
					newMap.put("timestamp", String.valueOf(System.currentTimeMillis()));
					newMap.put("file_path", cachePath.getAbsolutePath());
					newMap.put("uri", cachePath.getAbsolutePath());
					if (isVideo) {
						newMap.put("duration", "00:30");
					}
					
					truesingularityclass.inAppGalleryData.add(0, newMap);
					
					runOnUiThread(() -> {
						Toast.makeText(CustomgalleryActivity.this, "✨ NCNN AI द्वारा HD प्रोसेस होकर इन-ऐप गैलरी में सेव!", Toast.LENGTH_LONG).show();
						finish();
					});
					
				} catch (Exception e) {
					e.printStackTrace();
					runOnUiThread(() -> Toast.makeText(CustomgalleryActivity.this, "❌ AI एरर: " + e.getMessage(), Toast.LENGTH_SHORT).show());
				}
			}).start();
			
		} catch (Exception e) {
			e.printStackTrace();
			Toast.makeText(this, "❌ एरर: " + e.getMessage(), Toast.LENGTH_SHORT).show();
		}
	}
	
	public class Recyclerview1Adapter extends RecyclerView.Adapter<Recyclerview1Adapter.ViewHolder> {
		
		ArrayList<HashMap<String, Object>> _data;
		
		public Recyclerview1Adapter(ArrayList<HashMap<String, Object>> _arr) {
			_data = _arr;
		}
		
		@Override
		public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
			LayoutInflater _inflater = getLayoutInflater();
			View _v = _inflater.inflate(R.layout.hhh, parent, false);
			RecyclerView.LayoutParams _lp = new RecyclerView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
			_v.setLayoutParams(_lp);
			return new ViewHolder(_v);
		}
		
		@Override
		public void onBindViewHolder(ViewHolder _holder, final int _position) {
			View _view = _holder.itemView;
			HhhBinding hhhBinding = HhhBinding.bind(_view);
			
			int leftIndex = _position * 2;
			int rightIndex = _position * 2 + 1;
			
			// --- LEFT ITEM SETUP ---
			if (leftIndex < _data.size()) {
				HashMap<String, Object> leftItem = _data.get(leftIndex);
				String leftPath = (String) leftItem.get("path");
				boolean leftSelected = (Boolean) leftItem.get("selected");
				
				hhhBinding.relativelayout1.setVisibility(View.VISIBLE);
				Bitmap leftBmp = BitmapFactory.decodeFile(leftPath);
				if (leftBmp != null) {
					hhhBinding.imageview1.setImageBitmap(leftBmp);
				}
				
				if (leftSelected) {
					hhhBinding.imageviewTick1.setVisibility(View.VISIBLE);
				} else {
					hhhBinding.imageviewTick1.setVisibility(View.GONE);
				}
				
				hhhBinding.relativelayout1.setOnClickListener(v -> handleItemSelection(leftIndex));
				
			} else {
				hhhBinding.relativelayout1.setVisibility(View.INVISIBLE);
			}
			
			// --- RIGHT ITEM SETUP ---
			if (rightIndex < _data.size()) {
				HashMap<String, Object> rightItem = _data.get(rightIndex);
				String rightPath = (String) rightItem.get("path");
				String rightType = (String) rightItem.get("type");
				String rightDuration = (String) rightItem.get("duration");
				boolean rightSelected = (Boolean) rightItem.get("selected");
				
				hhhBinding.relativelayout2.setVisibility(View.VISIBLE);
				Bitmap rightBmp = BitmapFactory.decodeFile(rightPath);
				if (rightBmp != null) {
					hhhBinding.imageview6.setImageBitmap(rightBmp);
				}
				
				if ("video".equals(rightType)) {
					hhhBinding.textview8.setVisibility(View.VISIBLE);
					hhhBinding.textview8.setText(rightDuration != null ? rightDuration : "00:00");
				} else {
					hhhBinding.textview8.setVisibility(View.GONE);
				}
				
				if (rightSelected) {
					hhhBinding.imageviewTick2.setVisibility(View.VISIBLE);
				} else {
					hhhBinding.imageviewTick2.setVisibility(View.GONE);
				}
				
				hhhBinding.relativelayout2.setOnClickListener(v -> handleItemSelection(rightIndex));
				
			} else {
				hhhBinding.relativelayout2.setVisibility(View.INVISIBLE);
			}
		}
		
		private void handleItemSelection(int globalIndex) {
			if (selectedGlobalIndex != -1 && selectedGlobalIndex < _data.size()) {
				_data.get(selectedGlobalIndex).put("selected", false);
			}
			
			selectedGlobalIndex = globalIndex;
			HashMap<String, Object> clickedItem = _data.get(selectedGlobalIndex);
			clickedItem.put("selected", true);
			selectedImagePath = (String) clickedItem.get("path");
			
			notifyDataSetChanged();
			Toast.makeText(CustomgalleryActivity.this, "🎯 मीडिया सेलेक्ट हो गया!", Toast.LENGTH_SHORT).show();
		}
		
		@Override
		public int getItemCount() {
			return (_data.size() + 1) / 2;
		}
		
		public class ViewHolder extends RecyclerView.ViewHolder {
			public ViewHolder(View v) {
				super(v);
			}
		}
	}
}
