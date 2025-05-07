package com.ndk.build;

import android.app.Activity;
import android.widget.TextView;
import android.os.Bundle;


public class MainActivity extends Activity {

    public native String antik();

    
    static {
        System.loadLibrary("hello-jni");
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

        TextView textView = findViewById(R.id.mytest);
        textView.setText(antik());
    }
}
