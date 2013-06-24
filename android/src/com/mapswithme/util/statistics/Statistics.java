package com.mapswithme.util.statistics;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.os.Build;
import android.util.Log;

import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.api.MWMRequest;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.MathUtils;
import com.mapswithme.util.Utils;

public enum Statistics
{
  INSTANCE;

  private final static String TAG_PROMO_DE = "PROMO-DE: ";
  private static String TAG_API = "API: ";

  private final static String KEY_SESSIONS = "sessions";
  private final static String KEY_STAT_ENABLED = "stat_enabled";
  private final static String KEY_STAT_COLLECTED = "collected";

  private final static int ACTIVE_USER_MIN_SESSION = 2;
  private final static long ACTIVE_USER_MIN_TIME = 30*24*3600;

  // Statistics
  private EventBuilder mEventBuilder;
  private StatisticsEngine mStatisticsEngine;
  // Statistics params
  private boolean DEBUG = true;
  private boolean mNewSession = true;
  // Statistics counters
  private int mBookmarksCreated= 0;
  private int mSharedTimes = 0;


  private Statistics()
  {
    Log.d(TAG, "Created Statistics instance.");
  }

  private EventBuilder getEventBuilder()
  {
    return mEventBuilder;
  }

  public void trackIfEnabled(Context context, Event event)
  {
    if (isStatisticsEnabled(context))
      event.post();
  }

  public void trackCountryDownload(Context context)
  {
    trackIfEnabled(context, getEventBuilder().getSimpleNamedEvent("Country download"));
  }

  public void trackCountryUpdate(Context context)
  {
    trackIfEnabled(context, getEventBuilder().getSimpleNamedEvent("Country update"));
  }

  public void trackCountryDeleted(Context context)
  {
    trackIfEnabled(context, getEventBuilder().getSimpleNamedEvent("Country deleted"));
  }

  public void trackSearchCategoryClicked(Context context, String category)
  {
    final Event event = getEventBuilder().reset()
                          .setName("Search category clicked")
                          .addParam("category", category)
                          .getEvent();

    trackIfEnabled(context, event);
  }

  public void trackGroupChanged(Context context)
  {
    trackIfEnabled(context, getEventBuilder().getSimpleNamedEvent("Bookmark group changed"));
  }

  public void trackDescriptionChanged(Context context)
  {
    trackIfEnabled(context, getEventBuilder().getSimpleNamedEvent("Description changed"));
  }

  public void trackGroupCreated(Context context)
  {
    trackIfEnabled(context, getEventBuilder().getSimpleNamedEvent("Group Created"));
  }

  public void trackSearchContextChanged(Context context, String from, String to)
  {
    final Event event = getEventBuilder().reset()
                          .setName("Search context changed")
                          .addParam("from", from)
                          .addParam("to", to)
                          .getEvent();

    trackIfEnabled(context, event);
  }

  public void trackColorChanged(Context context, String from, String to)
  {
    final Event event = getEventBuilder().reset()
                          .setName("Color changed")
                          .addParam("from", from)
                          .addParam("to", to)
                          .getEvent();

    trackIfEnabled(context, event);
  }

  public void trackBookmarkCreated(Context context)
  {
    final Event event = getEventBuilder().reset()
                          .setName("Bookmark created")
                          .addParam("Count", String.valueOf(++mBookmarksCreated))
                          .getEvent();

    trackIfEnabled(context, event);
  }

  public void trackPlaceShared(Context context, String channel)
  {
    final Event event = getEventBuilder().reset()
                          .setName("Place Shared")
                          .addParam("Channel", channel)
                          .addParam("Count", String.valueOf(++mSharedTimes))
                          .getEvent();

    trackIfEnabled(context, event);
  }

  public void trackPromocodeDialogOpenedEvent()
  {
    getEventBuilder().getSimpleNamedEvent(TAG_PROMO_DE + "opened promo code dialog").post();
  }

  public void trackPromocodeActivatedEvent()
  {
    getEventBuilder().getSimpleNamedEvent(TAG_PROMO_DE + "promo code activated").post();
  }

  public void trackApiCall(MWMRequest request)
  {
    if (request != null && request.getCallerInfo() != null)
    {
      ensureConfigured(MWMApplication.get());
      //@formatter:off
     getEventBuilder().reset()
                      .setName("Api Called")
                      .addParam("Caller Package", request.getCallerInfo().packageName)
                      .getEvent()
                      .post();
     //@formatter:on
    }
  }

  public void startActivity(Activity activity)
  {
    ensureConfigured(activity);
    mStatisticsEngine.onStartSession(activity);
    registerSession(activity);
  }

  private void registerSession(Activity activity)
  {
    if (mNewSession)
    {
      final int currentSessionNumber = incAndGetSessionsNumber(activity);
      if (isStatisticsEnabled(activity) && isActiveUser(activity, currentSessionNumber))
      {
        // If we haven't collected yet
        // do it now.
        if (!isStatisticsCollected(activity))
          collectOneTimeStatistics(activity);
      }
      mNewSession = false;
    }
  }

  private void ensureConfigured(Context context)
  {
    if (mEventBuilder == null || mStatisticsEngine == null)
    {
      // Engine
      final String key = context.getResources().getString(R.string.flurry_app_key);
      mStatisticsEngine = new FlurryEngine(DEBUG, key);
      mStatisticsEngine.configure(null, null);
      // Builder
      mEventBuilder = new EventBuilder(mStatisticsEngine);
    }
  }

  public void stopActivity(Activity activity)
  {
    mStatisticsEngine.onEndSession(activity);
  }

  private void collectOneTimeStatistics(Activity activity)
  {
    final EventBuilder eventBuilder = getEventBuilder().reset();
    // Only for PRO
    if (MWMApplication.get().isProVersion())
    {
      // Number of sets
      BookmarkManager manager = BookmarkManager.getBookmarkManager(activity);
      final int categoriesCount = manager.getCategoriesCount();
      if (categoriesCount > 0)
      {
        // Calculate average num of bmks in category
        double[] sizes = new double[categoriesCount];
        for (int catIndex = 0; catIndex < categoriesCount; catIndex++)
          sizes[catIndex] = manager.getCategoryById(catIndex).getSize();
        final double average = MathUtils.average(sizes);

        eventBuilder.addParam("Average number of bmks", String.valueOf(average));
      }
      eventBuilder.addParam("Categories count", String.valueOf(categoriesCount))
                  .setName("One time PRO stat");

      trackIfEnabled(activity, eventBuilder.getEvent());
    }
    // TODO add number of maps
    setStatisticsCollected(activity, true);
  }


  private boolean isStatisticsCollected(Context context)
  {
    return MWMApplication.get().nativeGetBoolean(KEY_STAT_COLLECTED, false);
  }

  private void setStatisticsCollected(Context context, boolean isCollected)
  {
    MWMApplication.get().nativeSetBoolean(KEY_STAT_COLLECTED, isCollected);
  }

  public boolean isStatisticsEnabled(Context context)
  {
    return MWMApplication.get().nativeGetBoolean(KEY_STAT_ENABLED, true);
  }

  public void setStatEnabled(Context context, boolean isEnabled)
  {
    final MWMApplication app = MWMApplication.get();
    app.nativeSetBoolean(KEY_STAT_ENABLED, isEnabled);
    // We track if user turned on/off
    // statistics to understand data better.
    getEventBuilder().reset()
      .setName("Statistics status changed")
      .addParam("Enabled", String.valueOf(isEnabled))
      .getEvent()
      .post();
  }

  @TargetApi(Build.VERSION_CODES.GINGERBREAD)
  private boolean isActiveUser(Context context, int currentSessionNumber)
  {
    return currentSessionNumber >= ACTIVE_USER_MIN_SESSION
           || System.currentTimeMillis() - Utils.getInstallationTime(context) >= ACTIVE_USER_MIN_TIME;
  }

  private int incAndGetSessionsNumber(Context context)
  {
    final MWMApplication app = MWMApplication.get();
    final int currentSessionNumber = app.nativeGetInt(KEY_SESSIONS, 0) + 1;
    app.nativeSetInt(KEY_SESSIONS, currentSessionNumber);
    return currentSessionNumber;
  }

  private final static String TAG = "MWMStat";
}
