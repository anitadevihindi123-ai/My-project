package com.my.newproject;

import android.content.Context;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.view.HapticFeedbackConstants;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.FullScreenContentCallback;
import com.google.android.gms.ads.LoadAdError;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.ads.AdError;
import com.google.android.gms.ads.interstitial.InterstitialAd;
import com.google.android.gms.ads.interstitial.InterstitialAdLoadCallback;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.HashMap;
import com.my.newproject.databinding.ImejBinding;
import com.my.newproject.databinding.IiiiBinding;

public class ImejActivity extends AppCompatActivity {
	
	private ImejBinding binding;
	private ArrayList<HashMap<String, Object>> listMap = new ArrayList<>();
	
	private InterstitialAd mInterstitialAd1;
	private InterstitialAd mInterstitialAd2;
	private InterstitialAd mInterstitialAdEffect;

	private Bitmap currentSelectedBitmap = null;
	
	@Override
	protected void onCreate(Bundle _savedInstanceState) {
		super.onCreate(_savedInstanceState);
		getWindow().setFlags(
			android.view.WindowManager.LayoutParams.FLAG_SECURE,
			android.view.WindowManager.LayoutParams.FLAG_SECURE
		);

		binding = ImejBinding.inflate(getLayoutInflater());
		setContentView(binding.getRoot());
		
		MobileAds.initialize(this);
		initializeLogic();
		setupEffectClickListeners();
	}
	
   	private void initializeLogic() {
		// 1. पुरानी लिस्ट को खाली करो
		listMap.clear();

		// 2. Room डेटाबेस से सेव किया हुआ डेटा लोड करो
		AppDatabase db = AppDatabase.getDatabase(this);
		java.util.List<ImageEntity> savedList = db.imageDao().getAllImages();

		// 3. डेटाबेस के डेटा को लिस्ट में भरो
		if (savedList != null) {
			for (ImageEntity entity : savedList) {
				HashMap<String, Object> map = new HashMap<>();
				map.put("file_path", entity.filePath);
				map.put("type", entity.type);
				map.put("duration", entity.duration);
				listMap.add(map);
			}
		}

		// 4. रिसाइक्लर व्यू सेटअप करो
		binding.recyclerview1.setLayoutManager(new LinearLayoutManager(this));
		
		Recyclerview1Adapter adapter = new Recyclerview1Adapter(this, listMap);
		binding.recyclerview1.setAdapter(adapter);

		// 5. बटन के क्लिक्स
		binding.imageview1.setOnClickListener(v -> {
			v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
			android.content.Intent intent = new android.content.Intent(ImejActivity.this, MainActivity.class);
			startActivity(intent);
		});
		
		binding.imageview2.setOnClickListener(v -> {
			v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
			android.content.Intent intent = new android.content.Intent(ImejActivity.this, CustomgalleryActivity.class);
			startActivity(intent);
		});
	}


	private void setupEffectClickListeners() {
		binding.textview1.setOnClickListener(v -> {
			v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
			checkAndShowEffectAd(0xFF007F);
		});

		binding.textview2.setOnClickListener(v -> {
			v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
			checkAndShowEffectAd(0xFF7F00);
		});

		binding.textview3.setOnClickListener(v -> {
			v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
			checkAndShowEffectAd(0xFFD700);
		});

		binding.textview4.setOnClickListener(v -> {
			v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
			checkAndShowEffectAd(0x00E5FF);
		});

		binding.textview5.setOnClickListener(v -> {
			v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
			checkAndShowEffectAd(0xFF4081);
		});

		binding.textview7.setOnClickListener(v -> {
			v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
			checkAndShowEffectAd(0x7C4DFF);
		});

		binding.textview9.setOnClickListener(v -> {
			v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
			checkAndShowEffectAd(0x00B0FF);
		});

		binding.textview6.setOnClickListener(v -> {
			v.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
			checkAndShowEffectAd(0x00FF66);
		});
	}

	private void checkAndShowEffectAd(final int targetColorHex) {
		if (currentSelectedBitmap == null) {
			Toast.makeText(this, "⚠️ पहले रिसाइक्लर व्यू से किसी फोटो पर क्लिक करके सेलेक्ट करो!", Toast.LENGTH_LONG).show();
			return;
		}

		Toast.makeText(this, "विज्ञापन लोड हो रहा है...", Toast.LENGTH_SHORT).show();
		AdRequest adRequest = new AdRequest.Builder().build();

		InterstitialAd.load(this, "ca-app-pub-5020439862132022/3899369058", adRequest,
			new InterstitialAdLoadCallback() {
				@Override
				public void onAdLoaded(@NonNull InterstitialAd interstitialAd) {
					mInterstitialAdEffect = interstitialAd;
					mInterstitialAdEffect.show(ImejActivity.this);

					mInterstitialAdEffect.setFullScreenContentCallback(new FullScreenContentCallback() {
						@Override
						public void onAdDismissedFullScreenContent() {
							processTargetImage(targetColorHex);
						}

						@Override
						public void onAdFailedToShowFullScreenContent(@NonNull AdError adError) {
							Toast.makeText(ImejActivity.this, "❌ विज्ञापन दिखाने में समस्या आई!", Toast.LENGTH_LONG).show();
						}
					});
				}

				@Override
				public void onAdFailedToLoad(@NonNull LoadAdError loadAdError) {
					Toast.makeText(ImejActivity.this, "❌ इंटरनेट एरर (विज्ञापन लोड नहीं हुआ, कृपया इंटरनेट चालू करें)", Toast.LENGTH_LONG).show();
				}
			});
	}

	public void onImageItemClicked(Bitmap clickedBitmap) {
		this.currentSelectedBitmap = clickedBitmap;
		Toast.makeText(this, "🎯 फोटो सेलेक्ट हो गई है! अब नीचे से कोई इफ़ेक्ट चुनो।", Toast.LENGTH_SHORT).show();
	}

	private void processTargetImage(int targetColorHex) {
		Bitmap finalProcessedBitmap = applyBackgroundOnlyShadow(currentSelectedBitmap, targetColorHex);
		Toast.makeText(this, "✨ बैकग्राउंड कलर शैडो सफलतापूर्वक लग गया!", Toast.LENGTH_SHORT).show();
	}

	public Bitmap applyBackgroundOnlyShadow(Bitmap sourceBitmap, int targetColorHex) {
		int width = sourceBitmap.getWidth();
		int height = sourceBitmap.getHeight();
		
		Bitmap modifiedBitmap = sourceBitmap.copy(Bitmap.Config.ARGB_8888, true);
		
		int[] pixels = new int[width * height];
		modifiedBitmap.getPixels(pixels, 0, width, 0, 0, width, height);

		int targetR = (targetColorHex >> 16) & 0xFF;
		int targetG = (targetColorHex >> 8) & 0xFF;
		int targetB = targetColorHex & 0xFF;

		for (int i = 0; i < pixels.length; i++) {
			int pixel = pixels[i];
			
			int r = (pixel >> 16) & 0xFF;
			int g = (pixel >> 8) & 0xFF;
			int b = pixel & 0xFF;

			boolean isSkinTone = (r > 95 && g > 40 && b > 20 && 
								  (Math.max(r, Math.max(g, b)) - Math.min(r, Math.min(g, b))) > 15 && 
								  Math.abs(r - g) > 15 && r > g && r > b);

			if (!isSkinTone) {
				float blendFactor = 0.12f;
				
				int newR = (int) (r * (1 - blendFactor) + targetR * blendFactor);
				int newG = (int) (g * (1 - blendFactor) + targetG * blendFactor);
				int newB = (int) (b * (1 - blendFactor) + targetB * blendFactor);
				
				pixels[i] = (0xFF << 24) | (newR << 16) | (newG << 8) | newB;
			}
		}

		modifiedBitmap.setPixels(pixels, 0, width, 0, 0, width, height);
		return modifiedBitmap;
	}

	private void loadAndShowFirstAd(final String mediaType, final HashMap<String, Object> mediaItem, final int position) {
		Toast.makeText(this, "विज्ञापन लोड हो रहा है...", Toast.LENGTH_SHORT).show();
		AdRequest adRequest = new AdRequest.Builder().build();
		
		InterstitialAd.load(this, "ca-app-pub-5020439862132022/3899369058", adRequest,
			new InterstitialAdLoadCallback() {
				@Override
				public void onAdLoaded(@NonNull InterstitialAd interstitialAd) {
					mInterstitialAd1 = interstitialAd;
					mInterstitialAd1.show(ImejActivity.this);
					
					mInterstitialAd1.setFullScreenContentCallback(new FullScreenContentCallback() {
						@Override
						public void onAdDismissedFullScreenContent() {
							exportMediaToPhoneGalleryAndRemove(mediaType, mediaItem, position);
						}

						@Override
						public void onAdFailedToShowFullScreenContent(@NonNull AdError adError) {
							Toast.makeText(ImejActivity.this, "❌ विज्ञापन दिखाने में समस्या आई, सेव नहीं होगा!", Toast.LENGTH_LONG).show();
						}
					});
				}

				@Override
				public void onAdFailedToLoad(@NonNull LoadAdError loadAdError) {
					Toast.makeText(ImejActivity.this, "❌ इंटरनेट एरर! कृपया इंटरनेट चालू करें, सेव नहीं होगा।", Toast.LENGTH_LONG).show();
				}
			});
	}

	private void exportMediaToPhoneGalleryAndRemove(String type, HashMap<String, Object> item, int position) {
		try {
			String filePath = (String) item.get("file_path");
			if (filePath == null) {
				filePath = (String) item.get("uri");
			}

			if (filePath != null) {
				File sourceFile = new File(filePath);
				
				if ("video".equals(type)) {
					android.content.ContentValues values = new android.content.ContentValues();
					values.put(android.provider.MediaStore.Video.Media.DISPLAY_NAME, "Singularity_Video_" + System.currentTimeMillis() + ".mp4");
					values.put(android.provider.MediaStore.Video.Media.MIME_TYPE, "video/mp4");
					values.put(android.provider.MediaStore.Video.Media.RELATIVE_PATH, android.os.Environment.DIRECTORY_MOVIES + "/NewProject");

					android.net.Uri uri = getContentResolver().insert(android.provider.MediaStore.Video.Media.EXTERNAL_CONTENT_URI, values);
					if (uri != null) {
						try (OutputStream out = getContentResolver().openOutputStream(uri);
							 InputStream in = sourceFile.exists() ? new FileInputStream(sourceFile) : null) {
							if (out != null && in != null) {
								byte[] buffer = new byte[4096];
								int length;
								while ((length = in.read(buffer)) > 0) {
									out.write(buffer, 0, length);
								}
							}
						}
					}
					Toast.makeText(this, "🚀 वीडियो गैलरी में सेव हो गया!", Toast.LENGTH_LONG).show();

				} else {
					android.content.ContentValues values = new android.content.ContentValues();
					values.put(android.provider.MediaStore.Images.Media.DISPLAY_NAME, "Singularity_Photo_" + System.currentTimeMillis() + ".jpg");
					values.put(android.provider.MediaStore.Images.Media.MIME_TYPE, "image/jpeg");
					values.put(android.provider.MediaStore.Images.Media.RELATIVE_PATH, android.os.Environment.DIRECTORY_PICTURES + "/NewProject");

					android.net.Uri uri = getContentResolver().insert(android.provider.MediaStore.Images.Media.EXTERNAL_CONTENT_URI, values);
					if (uri != null) {
						try (OutputStream out = getContentResolver().openOutputStream(uri);
							 InputStream in = sourceFile.exists() ? new FileInputStream(sourceFile) : null) {
							if (out != null && in != null) {
								byte[] buffer = new byte[4096];
								int length;
								while ((length = in.read(buffer)) > 0) {
									out.write(buffer, 0, length);
								}
							}
						}
					}
					Toast.makeText(this, "🔥 फोटो गैलरी में सेव हो गया!", Toast.LENGTH_LONG).show();
				}

				if (sourceFile.exists()) {
					sourceFile.delete();
				}
			}

			if (truesingularityclass.inAppGalleryData != null && position >= 0 && position < truesingularityclass.inAppGalleryData.size()) {
				truesingularityclass.inAppGalleryData.remove(position);
			}

			if (binding.recyclerview1.getAdapter() != null) {
				binding.recyclerview1.getAdapter().notifyDataSetChanged();
			}

		} catch (Exception e) {
			e.printStackTrace();
			Toast.makeText(this, "❌ एरर: " + e.getMessage(), Toast.LENGTH_SHORT).show();
		}
	}
	
	public class Recyclerview1Adapter extends RecyclerView.Adapter<Recyclerview1Adapter.ViewHolder> {
		
		ArrayList<HashMap<String, Object>> _data;
		Context context;
		
		public Recyclerview1Adapter(Context context, ArrayList<HashMap<String, Object>> _arr) {
			this.context = context;
			_data = _arr;
		}
		
		@Override
		public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
			LayoutInflater _inflater = LayoutInflater.from(parent.getContext());
			View _v = _inflater.inflate(R.layout.iiii, parent, false);
			RecyclerView.LayoutParams _lp = new RecyclerView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
			_v.setLayoutParams(_lp);
			return new ViewHolder(_v);
		}
		
		@Override
		public void onBindViewHolder(ViewHolder _holder, final int _position) {
			View _view = _holder.itemView;
			IiiiBinding iiiibinding = IiiiBinding.bind(_view);
			
			HashMap<String, Object> item = _data.get(_position);
			
			String type = (String) item.get("type");
			String duration = (String) item.get("duration");
			
			iiiibinding.textDelete1.setVisibility(View.GONE);
			iiiibinding.textDelete2.setVisibility(View.GONE);
			
			if ("video".equals(type)) {
				if (iiiibinding.textview8 != null) {
					iiiibinding.textview8.setVisibility(View.VISIBLE);
					iiiibinding.textview8.setText(duration != null ? duration : "00:00");
				}
			} else {
				if (iiiibinding.textview8 != null) {
					iiiibinding.textview8.setVisibility(View.GONE);
				}
			}
			
			_view.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					if ("video".equals(type)) {
						iiiibinding.textDelete2.setVisibility(View.VISIBLE);
						iiiibinding.textDelete1.setVisibility(View.GONE);
					} else {
						iiiibinding.textDelete1.setVisibility(View.VISIBLE);
						iiiibinding.textDelete2.setVisibility(View.GONE);
						
						try {
							iiiibinding.imageview1.setDrawingCacheEnabled(true);
							Bitmap bmp = Bitmap.createBitmap(iiiibinding.imageview1.getDrawingCache());
							iiiibinding.imageview1.setDrawingCacheEnabled(false);
							onImageItemClicked(bmp);
						} catch (Exception e) {
							e.printStackTrace();
						}
					}
				}
			});
			
			iiiibinding.textDelete1.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					int currentPos = _holder.getAdapterPosition();
					if (currentPos != RecyclerView.NO_POSITION && currentPos < _data.size()) {
						    HashMap<String, Object> map = _data.get(currentPos);
    String filePath = (String) map.get("file_path");
    if (filePath == null) filePath = (String) map.get("uri");
    
    if (filePath != null) {
        final String pathToDelete = filePath;
        new Thread(() -> {
            try {
                AppDatabase db = AppDatabase.getDatabase(context);
                db.imageDao().deleteImageByPath(pathToDelete);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }).start();
    }
_data.remove(currentPos);
						notifyItemRemoved(currentPos);
						notifyItemRangeChanged(currentPos, _data.size());
						Toast.makeText(context, "🗑️ फोटो डिलीट हो गई!", Toast.LENGTH_SHORT).show();
					}
				}
			});

			iiiibinding.textDelete2.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					int currentPos = _holder.getAdapterPosition();
					if (currentPos != RecyclerView.NO_POSITION && currentPos < _data.size()) {
						    HashMap<String, Object> map = _data.get(currentPos);
    String filePath = (String) map.get("file_path");
    if (filePath == null) filePath = (String) map.get("uri");
    
    if (filePath != null) {
        final String pathToDelete = filePath;
        new Thread(() -> {
            try {
                AppDatabase db = AppDatabase.getDatabase(context);
                db.imageDao().deleteImageByPath(pathToDelete);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }).start();
    }
_data.remove(currentPos);
						notifyItemRemoved(currentPos);
						notifyItemRangeChanged(currentPos, _data.size());
						Toast.makeText(context, "🗑️ वीडियो डिलीट हो गया!", Toast.LENGTH_SHORT).show();
					}
				}
			});
			
			iiiibinding.textview1.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					int currentPos = _holder.getAdapterPosition();
					if (currentPos != RecyclerView.NO_POSITION && currentPos < _data.size()) {
						loadAndShowFirstAd(type, _data.get(currentPos), currentPos);
					}
				}
			});

			if (iiiibinding.textview9 != null) {
				iiiibinding.textview9.setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View v) {
						int currentPos = _holder.getAdapterPosition();
						if (currentPos != RecyclerView.NO_POSITION && currentPos < _data.size()) {
							loadAndShowFirstAd(type, _data.get(currentPos), currentPos);
						}
					}
				});
			}
		}
		
		@Override
		public int getItemCount() {
			return _data.size();
		}
		
		public class ViewHolder extends RecyclerView.ViewHolder {
			public ViewHolder(View v) {
				super(v);
			}
		}
	}
}
